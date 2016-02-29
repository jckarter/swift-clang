#include "clang-service/Service.h"
#include "clang-service/SafeLibclang.h"
#include "clang-service/Support/LiveData.h"
#include "clang-service/Support/ValueUtils.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"

#include <fstream>
#include <mutex>

using llvm::make_unique;
using llvm::StringRef;
using LockGuard = std::unique_lock<std::mutex>;
using namespace Libclang;

namespace ClangService {

namespace {
#define REGISTER_UID(Name, UIDStr) LazyCSUID Name{UIDStr};
#include "clang-service/ProtocolUIDs.inc"
}

namespace {

Value makeError(ErrorKind EK, StringRef Description) {
  auto IOS = InlineOwnedString::create(1 + Description.size() + 1);
  {
    raw_inlinestring_ostream Stream{*IOS.get()};
    Stream << char(EK);
    Stream << Description;
    Stream << '\0';
  }
  return Value::data(std::move(IOS));
}

Value makeRequestInvalidError(StringRef Description) {
  return makeError(ErrorKind::RequestInvalid, Description);
}

Value makeRequestFailedError(StringRef Description) {
  return makeError(ErrorKind::RequestFailed, Description);
}

Value makeLibclangFailedError(CXErrorCode EC) {
  switch (EC) {
  case CXError_Success:
    return makeRequestFailedError("Libclang succeeded");
  case CXError_Failure:
    return makeRequestFailedError("Libclang failed");
  case CXError_Crashed:
    return makeRequestFailedError("Libclang crashed");
  case CXError_InvalidArguments:
    return makeRequestFailedError("Invalid arguments to a libclang API");
  case CXError_ASTReadError:
    return makeRequestFailedError("Libclang couldn't read an AST");
  }
}

#if 0
Value makeRequestInterruptedError(StringRef Description) {
  return makeError(ErrorKind::RequestInterrupted, Description);
}

Value makeRequestCancelledError(StringRef Description) {
  return makeError(ErrorKind::RequestCancelled, Description);
}
#endif

} // end anonymous namespace

struct TokenGenerator {
  uint64_t NextToken;
  llvm::SmallVector<uint64_t, 4> UnusedTokens;

  TokenGenerator() : NextToken(0), UnusedTokens() {}

  uint64_t getUniqueToken() {
    if (!UnusedTokens.empty()) {
      uint64_t Token = UnusedTokens.back();
      UnusedTokens.pop_back();
      return Token;
    }
    return NextToken++;
  }

  void recycleToken(uint64_t Token) { UnusedTokens.push_back(Token); }
};

template <typename T> using TokenMap = llvm::SmallDenseMap<uint64_t, T>;

struct CodeCompletionContext {
  SafeCXIndex Idx;

  CodeCompletionContext(SafeCXIndex Idx) : Idx(std::move(Idx)) {}
};

struct IndexingContext {
  ServiceState *State;
  SafeCXIndex Idx;

  IndexingContext(ServiceState *State, SafeCXIndex Idx)
      : State(State), Idx(std::move(Idx)) {}
};

struct ServiceState {
  std::mutex LogLock;

  std::mutex CodeCompletionCtxsLock;
  TokenGenerator CodeCompletionTokenGen;
  TokenMap<std::unique_ptr<CodeCompletionContext>> CodeCompletionCtxs;

  std::mutex IndexingCtxsLock;
  TokenGenerator IndexingTokenGen;
  TokenMap<std::unique_ptr<IndexingContext>> IndexingCtxs;
};

Service::Service() : State(new ServiceState) {}

Service::~Service() { delete State; }

void Service::serve() {
  assert(!"Unreachable");
  abort();
}

namespace {

/// UniqueCXString ::= string (Optional)
///
/// Note: This routine assumes that the string will live long enough to be
/// transferred.
Value toValue(UniqueCXString S) {
  if (clang_getCString(S))
    return Value::string(make_unique<OwnedCXString>(std::move(S)));
  return {};
}

/// CXFile ::= {
///   key.location.file.name: string,
///   key.location.file.uid: data (Optional)
/// }
Value toValue(CXFile File) {
  UniqueCXString Name{clang_getFileName(File)};
  Value VName = toValue(std::move(Name));

  CXFileUniqueID FUID;
  Value VFUID;
  if (!clang_getFileUniqueID(File, &FUID)) {
    auto IOS = InlineOwnedString::create(
        {(const char *)&FUID, sizeof(CXFileUniqueID)});
    VFUID = Value::data(std::move(IOS));
  }

  return Value::dict({{KeyLocationFileName, std::move(VName)},
                      {KeyLocationFileUID, std::move(VFUID)}});
}

// FIXME: Split this up into something smaller.
/// CXSourceLocation ::= {
///   key.location.in_sys_header: bool,
///   key.location.from_main_file: bool,
///
///   key.location.expansion.file: CXFile,
///   key.location.expansion.line: int64,
///   key.location.expansion.column: int64,
///   key.location.expansion.offset: int64,
///
///   key.location.presumed.filename: string,
///   key.location.presumed.line: int64,
///   key.location.presumed.column: int64,
///
///   key.location.instantiation.file: CXFile,
///   key.location.instantiation.line: int64,
///   key.location.instantiation.column: int64,
///   key.location.instantiation.offset: int64,
///
///   key.location.spelling.file: CXFile,
///   key.location.spelling.line: int64,
///   key.location.spelling.column: int64,
///   key.location.spelling.offset: int64
///
///   key.location.file.file: CXFile,
///   key.location.file.line: int64,
///   key.location.file.column: int64,
///   key.location.file.offset: int64
/// }
Value toValue(CXSourceLocation Loc) {
  bool in_sys_header = clang_Location_isInSystemHeader(Loc);
  bool from_main_file = clang_Location_isFromMainFile(Loc);

  CXFile expansion_file;
  unsigned expansion_line;
  unsigned expansion_col;
  unsigned expansion_offset;
  clang_getExpansionLocation(Loc, &expansion_file, &expansion_line,
                             &expansion_col, &expansion_offset);

  UniqueCXString presumed_filename{/*unsafe!=*/{}};
  unsigned presumed_line;
  unsigned presumed_col;
  clang_getPresumedLocation(Loc, presumed_filename.storage(), &presumed_line,
                            &presumed_col);
  Value Vpresumed_filename = toValue(std::move(presumed_filename));

  CXFile inst_file;
  unsigned inst_line;
  unsigned inst_col;
  unsigned inst_offset;
  clang_getInstantiationLocation(Loc, &inst_file, &inst_line, &inst_col,
                                 &inst_offset);

  CXFile spell_file;
  unsigned spell_line;
  unsigned spell_col;
  unsigned spell_offset;
  clang_getSpellingLocation(Loc, &spell_file, &spell_line, &spell_col,
                            &spell_offset);

  CXFile file;
  unsigned line;
  unsigned col;
  unsigned offset;
  clang_getFileLocation(Loc, &file, &line, &col, &offset);

  return Value::dict({
      {KeyLocationInSysHeader, in_sys_header},
      {KeyLocationFromMainFile, from_main_file},

      {KeyLocationExpansionFile, toValue(expansion_file)},
      {KeyLocationExpansionLineOffset, int64_t(expansion_line)},
      {KeyLocationExpansionColOffset, int64_t(expansion_col)},
      {KeyLocationExpansionOffset, int64_t(expansion_offset)},

      {KeyLocationPresumedFilename, std::move(Vpresumed_filename)},
      {KeyLocationPresumedLineOffset, int64_t(presumed_line)},
      {KeyLocationPresumedColOffset, int64_t(presumed_col)},

      {KeyLocationInstantiationFile, toValue(inst_file)},
      {KeyLocationInstantiationLineOffset, int64_t(inst_line)},
      {KeyLocationInstantiationColOffset, int64_t(inst_col)},
      {KeyLocationInstantiationOffset, int64_t(inst_offset)},

      {KeyLocationSpellingFile, toValue(spell_file)},
      {KeyLocationSpellingLineOffset, int64_t(spell_line)},
      {KeyLocationSpellingColOffset, int64_t(spell_col)},
      {KeyLocationSpellingOffset, int64_t(spell_offset)},

      {KeyLocationFileFile, toValue(file)},
      {KeyLocationFileLineOffset, int64_t(line)},
      {KeyLocationFileColOffset, int64_t(col)},
      {KeyLocationFileOffset, int64_t(offset)}});
}

/// CXSourceRange ::= {
///   key.range.start: CXSourceLocation,
///   key.range.end: CXSourceLocation
/// }
Value toValue(CXSourceRange &Range) {
  CXSourceLocation Start = clang_getRangeStart(Range);
  CXSourceLocation End = clang_getRangeEnd(Range);
  return Value::dict(
      {{KeyRangeStart, toValue(Start)}, {KeyRangeEnd, toValue(End)}});
}

/// CXDiagnostic ::= {
///   key.diagnostic.severity: int64,
///   key.diagnostic.string: string,
///   key.diagnostic.location: CXSourceLocation,
///   key.diagnostic.fixits: [[string, CXSourceRange]]
/// }
///
/// Note: This routine assumes that all data referred to by the diagnostic will
/// live long enough to be transferred.
Value toValue(CXDiagnostic Diagnostic,
              CXDiagnosticDisplayOptions DiagnosticOpts) {
  CXDiagnosticSeverity Severity = clang_getDiagnosticSeverity(Diagnostic);

  UniqueCXString DiagStr{clang_formatDiagnostic(Diagnostic, DiagnosticOpts)};
  auto VDiagStr = toValue(std::move(DiagStr));

  CXSourceLocation SourceLoc = clang_getDiagnosticLocation(Diagnostic);

  auto VFixits = Value::array({});
  auto &AFixits = VFixits.getArray();
  unsigned NumFixIts = clang_getDiagnosticNumFixIts(Diagnostic);
  AFixits.reserve(NumFixIts);
  for (unsigned I = 0; I < NumFixIts; ++I) {
    CXSourceRange Replacement;
    UniqueCXString Fixit{clang_getDiagnosticFixIt(Diagnostic, I, &Replacement)};
    AFixits.push_back(
        Value::array({toValue(std::move(Fixit)), toValue(Replacement)}));
  }

  return Value::dict({{KeyDiagnosticSeverity, int64_t(Severity)},
                      {KeyDiagnosticString, std::move(VDiagStr)},
                      {KeyDiagnosticLocation, toValue(SourceLoc)},
                      {KeyDiagnosticFixits, std::move(VFixits)}});
}

/// CXCompletionString ::= {
///   key.codecomplete.completion.str.kind: int64 (Optional),
///   key.codecomplete.completion.str.text: string (Optional),
///   key.codecomplete.completion.str.chunks: [CXCompletionString],
///   key.codecomplete.completion.str.availability: int64,
///   key.codecomplete.completion.str.priority: int64,
///   key.codecomplete.completion.str.annotations: [string],
///   key.codecomplete.completion.str.parent: string (Optional),
///   key.codecomplete.completion.str.brief: string (Optional)
/// }
Value toValue(CXCompletionString CompletionStr, bool IncludeBrief) {
  unsigned NumChunks = clang_getNumCompletionChunks(CompletionStr);

  auto VChunks = Value::array({});
  auto &AChunks = VChunks.getArray();
  AChunks.reserve(NumChunks);

  for (unsigned I = 0; I < NumChunks; ++I) {
    CXCompletionString SubCompletion =
        clang_getCompletionChunkCompletionString(CompletionStr, I);
    auto VSubCompletion = toValue(SubCompletion, IncludeBrief);

    auto &DSubCompletion = VSubCompletion.getDict();

    CXCompletionChunkKind Kind = clang_getCompletionChunkKind(CompletionStr, I);
    DSubCompletion[KeyCodeCompleteCompletionStrKind] = int64_t(Kind);

    UniqueCXString SubText{clang_getCompletionChunkText(CompletionStr, I)};
    DSubCompletion[KeyCodeCompleteCompletionStrText] =
        toValue(std::move(SubText));

    AChunks.push_back(std::move(VSubCompletion));
  }

  CXAvailabilityKind Availability =
      clang_getCompletionAvailability(CompletionStr);

  unsigned Priority = clang_getCompletionPriority(CompletionStr);

  unsigned NumAnnotations = clang_getCompletionNumAnnotations(CompletionStr);
  auto VAnnotations = Value::array({});
  auto &AAnnotations = VAnnotations.getArray();
  AAnnotations.reserve(NumAnnotations);
  for (unsigned I = 0; I < NumAnnotations; ++I) {
    UniqueCXString Annotation{clang_getCompletionAnnotation(CompletionStr, I)};
    AAnnotations.push_back(toValue(std::move(Annotation)));
  }

  CXCursorKind ParentKind;
  UniqueCXString Parent{clang_getCompletionParent(CompletionStr, &ParentKind)};
  auto VParent = toValue(std::move(Parent));

  Value VBrief;
  if (IncludeBrief) {
    UniqueCXString Brief{clang_getCompletionBriefComment(CompletionStr)};
    VBrief = toValue(std::move(Brief));
  }

  return Value::dict(
      {{KeyCodeCompleteCompletionStrChunks, std::move(VChunks)},
       {KeyCodeCompleteCompletionStrAvailability, int64_t(Availability)},
       {KeyCodeCompleteCompletionStrPriority, int64_t(Priority)},
       {KeyCodeCompleteCompletionStrAnnotations, std::move(VAnnotations)},
       {KeyCodeCompleteCompletionStrParent, std::move(VParent)},
       {KeyCodeCompleteCompletionStrParentKind, int64_t(ParentKind)},
       {KeyCodeCompleteCompletionStrBrief, std::move(VBrief)}});
}

/// CXCompletionResult ::= {
///   key.codecomplete.completion.kind: int64,
///   key.codecomplete.completion.str: CXCompletionString
/// }
Value toValue(CXCompletionResult *Completion, bool IncludeBrief) {
  return Value::dict(
      {{KeyCodeCompleteCompletionKind, int64_t(Completion->CursorKind)},
       {KeyCodeCompleteCompletionStr,
        toValue(Completion->CompletionString, IncludeBrief)}});
}

/// CXCodeCompleteResults ::= {
///   key.token: int64,
///   key.codecomplete.results: [CXCompletionResult],
///   key.codecomplete.contexts: int64,
///   key.codecomplete.container.kind: int64,
///   key.codecomplete.container.incomplete: bool,
///   key.codecomplete.container.usr: string (Optional),
///   key.codecomplete.objc_selector: string (Optional),
///   key.codecomplete.diagnostics: [CXDiagnostic] (Optional)
/// }
Value toValue(CXCodeCompleteResults *Results, uint64_t Token, bool DoSort,
              bool IncludeBrief, bool DoDiagnostics,
              CXDiagnosticDisplayOptions DiagnosticOpts) {
  if (DoSort)
    clang_sortCodeCompletionResults(Results->Results, Results->NumResults);

  auto VResults = Value::array({});
  auto &AResults = VResults.getArray();
  AResults.reserve(Results->NumResults);
  for (unsigned I = 0; I < Results->NumResults; ++I)
    AResults.push_back(toValue(Results->Results + I, IncludeBrief));

  unsigned long long Contexts = clang_codeCompleteGetContexts(Results);

  unsigned Incomplete;
  CXCursorKind ContainerKind =
      clang_codeCompleteGetContainerKind(Results, &Incomplete);

  Value VUSR;
  if (ContainerKind != CXCursor_InvalidCode) {
    UniqueCXString USR{clang_codeCompleteGetContainerUSR(Results)};
    VUSR = toValue(std::move(USR));
  }

  UniqueCXString ObjCSelector{clang_codeCompleteGetObjCSelector(Results)};
  Value VObjCSelector = toValue(std::move(ObjCSelector));

  Value Diagnostics;
  if (DoDiagnostics) {
    auto VDiagnostics = Value::array({});
    auto &ADiagnostics = VDiagnostics.getArray();
    unsigned NumDiagnostics = clang_codeCompleteGetNumDiagnostics(Results);
    ADiagnostics.reserve(NumDiagnostics);
    for (unsigned I = 0; I < NumDiagnostics; ++I) {
      UniqueCXDiagnostic Diag{clang_codeCompleteGetDiagnostic(Results, I)};
      ADiagnostics.push_back(toValue(Diag, DiagnosticOpts));
    }
    Diagnostics = std::move(VDiagnostics);
  }

  return Value::dict({{KeyToken, int64_t(Token)},
                      {KeyCodeCompleteResults, std::move(VResults)},
                      {KeyCodeCompleteContexts, int64_t(Contexts)},
                      {KeyCodeCompleteContainerKind, int64_t(ContainerKind)},
                      {KeyCodeCompleteContainerIncomplete, bool(Incomplete)},
                      {KeyCodeCompleteContainerUSR, std::move(VUSR)},
                      {KeyCodeCompleteObjCSelector, std::move(VObjCSelector)},
                      {KeyCodeCompleteDiagnostics, std::move(Diagnostics)}});
}

} // end anonymous namespace

namespace {

int handleIndexerAbortQueryEvent(CXClientData CD, void *) {
  return -1 /* abort! */;
}

void handleIndexerDiagnosticEvent(CXClientData CD, CXDiagnosticSet DS, void *) {
}

CXIdxClientFile handleIndexerEnteredMainFileEvent(CXClientData CD,
                                                  CXFile MainFile, void *) {
  return nullptr;
}

CXIdxClientFile handleIndexerPPIncludeEvent(CXClientData CD,
                                            const CXIdxIncludedFileInfo *IFI) {
  return nullptr;
}

CXIdxClientASTFile
handleIndexerImportedASTEvent(CXClientData CD,
                              const CXIdxImportedASTFileInfo *IAFI) {
  return nullptr;
}

CXIdxClientContainer handleIndexerStartedTUEvent(CXClientData CD, void *) {
  return nullptr;
}

void handleIndexerDeclarationEvent(CXClientData CD, const CXIdxDeclInfo *IDI) {}

void handleIndexerEntityReferenceEvent(CXClientData CD,
                                       const CXIdxEntityRefInfo *ERI) {}

} // end anonymous namespace

class ResponseBuilder {
  /// State needed to handle requests.
  ServiceState *State;
  LiveData &LD;

  /// State needed to describe a request.
  Value::Dict &ClientDict;
  StringRef RequestStr;
  CSUID RequestUID;

  /// Parameters needed by various handlers.
  uint64_t ReqToken;
  StringRef Filename;
  uint64_t LineOffset;
  uint64_t ColOffset;
  llvm::SmallVector<const char *, 8> CmdArgs;
  llvm::SmallVector<CXUnsavedFile, 4> UnsavedFiles;

  /// Optional parameters used to control libclang's behavior.
  CXGlobalOptFlags IndexOpts;
  unsigned IndexExcludeDeclsFromPCH;
  unsigned IndexDiagnostics;
  CXIndexOptFlags IndexSourceOpts;
  bool DoDiagnostics;
  CXDiagnosticDisplayOptions DiagnosticOpts;
  unsigned ParseOpts;
  bool IncludeBrief;
  unsigned CodeCompleteOpts;
  bool DoSort;
  unsigned ReparseOpts;

  /// Set ReqToken.
  bool setKeyToken() {
    Value &ClientKeyToken = ClientDict[KeyToken];
    if (ClientKeyToken.isInt64())
      ReqToken = (uint64_t)ClientKeyToken.getInt64();
    return ClientKeyToken.isInt64();
  }

  /// Set Filename.
  bool setKeyName() {
    Value &ClientKeyName = ClientDict[KeyName];
    if (ClientKeyName.isStringLike())
      Filename = ClientKeyName.getStringRef();
    return ClientKeyName.isStringLike();
  }

  // Set LineOffset.
  bool setKeyLineOffset() {
    Value &ClientKeyLineOffset = ClientDict[KeyLineOffset];
    if (ClientKeyLineOffset.isInt64())
      LineOffset = uint64_t(ClientKeyLineOffset.getInt64());
    return ClientKeyLineOffset.isInt64();
  }

  /// Set ColOffset.
  bool setKeyColOffset() {
    Value &ClientKeyColOffset = ClientDict[KeyColOffset];
    if (ClientKeyColOffset.isInt64())
      ColOffset = uint64_t(ClientKeyColOffset.getInt64());
    return ClientKeyColOffset.isInt64();
  }

  /// Set CmdArgs (Optional).
  void setKeyCmdArgs() {
    Value &ClientKeyCmdArgs = ClientDict[KeyCmdArgs];
    if (ClientKeyCmdArgs.isArray())
      toStringArray(ClientKeyCmdArgs, CmdArgs);
    else
      CmdArgs.clear();
  }

  /// Set UnsavedFiles (Optional).
  void setKeyFiles() {
    /// key.files
    Value &ClientKeyFiles = ClientDict[KeyFiles];
    if (ClientKeyFiles.isArray())
      toUnsavedFilesArray(ClientKeyFiles, UnsavedFiles);
    else
      UnsavedFiles.clear();
  }

  /// Set the parameters needed to create CXIndex objects.
  void setCXIndexParameters() {
    Value &ClientKeyIndexOptions = ClientDict[KeyIndexOptions];
    IndexOpts = ClientKeyIndexOptions.isInt64()
                    ? (CXGlobalOptFlags)ClientKeyIndexOptions.getInt64()
                    : CXGlobalOpt_None;

    Value &ClientKeyIndexExcludeDeclsFromPCH =
        ClientDict[KeyIndexExcludeDeclsFromPCH];
    IndexExcludeDeclsFromPCH = ClientKeyIndexExcludeDeclsFromPCH.isBool()
                                   ? ClientKeyIndexExcludeDeclsFromPCH.getBool()
                                   : 0;

    Value &ClientKeyIndexDiagnostics = ClientDict[KeyIndexDiagnostics];
    IndexDiagnostics = ClientKeyIndexDiagnostics.isBool()
                           ? ClientKeyIndexDiagnostics.getBool()
                           : 0;
  }

  /// Set the parameters needed to control indexing options.
  void setKeyIndexSourceOptions() {
    Value &ClientKeyIndexSourceOptions = ClientDict[KeyIndexSourceOptions];
    IndexSourceOpts =
        ClientKeyIndexSourceOptions.isInt64()
            ? (CXIndexOptFlags)ClientKeyIndexSourceOptions.getInt64()
            : CXIndexOpt_None;
  }

  /// Set the parameters needed to generate (or skip) diagnostics.
  void setDiagnosticsParameters() {
    Value &ClientKeyDiagnosticsEnabled = ClientDict[KeyDiagnosticsEnabled];
    DoDiagnostics = ClientKeyDiagnosticsEnabled.isBool()
                        ? ClientKeyDiagnosticsEnabled.getBool()
                        : false;

    Value &ClientKeyDiagnosticsOptions = ClientDict[KeyDiagnosticsOptions];
    DiagnosticOpts = (CXDiagnosticDisplayOptions)(
        ClientKeyDiagnosticsOptions.isInt64()
            ? ClientKeyDiagnosticsOptions.getInt64()
            : 0);
    if (DiagnosticOpts)
      DoDiagnostics = true;
    if (DoDiagnostics && !DiagnosticOpts)
      DiagnosticOpts = (CXDiagnosticDisplayOptions)(
          CXDiagnostic_DisplaySourceLocation | CXDiagnostic_DisplayColumn |
          CXDiagnostic_DisplaySourceRanges | CXDiagnostic_DisplayOption);
  }

  /// Set the parameters passed to 'parseTranslationUnit' and friends.
  void setParseParameters() {
    Value &ParseOptions = ClientDict[KeyParseOptions];
    if (ParseOptions.isInt64())
      ParseOpts = ParseOptions.getInt64();
    else
      ParseOpts = CXTranslationUnit_DetailedPreprocessingRecord |
                  clang_defaultEditingTranslationUnitOptions();

    IncludeBrief =
        ParseOpts & CXTranslationUnit_IncludeBriefCommentsInCodeCompletion;
  }

  /// Set the parameters passed to codeCompleteAt.
  void setCodeCompleteParameters() {
    Value &CodeCompleteOptions = ClientDict[KeyCodeCompleteOptions];
    if (CodeCompleteOptions.isInt64())
      CodeCompleteOpts = CodeCompleteOptions.getInt64();
    else
      CodeCompleteOpts = clang_defaultCodeCompleteOptions();

    Value &ClientKeySort = ClientDict[KeySort];
    DoSort = ClientKeySort.isBool() && ClientKeySort.getBool();
  }

  /// Set the parameters passed to 'reparseTranslationUnit'.
  void setReparseParameters(UniqueCXTranslationUnit &TU) {
    // FIXME: Expose this to clients.
    ReparseOpts = clang_defaultReparseOptions(TU);
  }

  /// Create a SafeCXIndex (after setting any user-provided parameters).
  SafeCXIndex createSafeCXIndex() {
    setCXIndexParameters();
    SafeCXIndex Idx{
        clang_createIndex(IndexExcludeDeclsFromPCH, IndexDiagnostics)};
    clang_CXIndex_setGlobalOptions(Idx, IndexOpts);
    return Idx;
  }

  Value handleRequestVersion() {
    /// RequestVersion ::= {}
    ///
    /// Version ::= {
    ///   key.version_major: int64,
    ///   key.version_minor: int64
    /// }
    return Value::dict({{KeyVersionMajor, int64_t(CINDEX_VERSION_MAJOR)},
                        {KeyVersionMinor, int64_t(CINDEX_VERSION_MINOR)}});
  }

  // FIXME: Add support for code completion with existing TU's. Do this by
  // requiring clients to pass in a TU token.
  Value handleRequestCodeComplete() {
    if (RequestUID == RequestCodeCompleteOpen) {
      if (!setKeyName() || !setKeyLineOffset() || !setKeyColOffset())
        return makeRequestInvalidError("Expected: CodeComplete init");
    } else if (RequestUID == RequestCodeComplete ||
               RequestUID == RequestCodeCompleteUpdate) {
      if (!setKeyLineOffset() || !setKeyColOffset() || !setKeyToken())
        return makeRequestInvalidError("Expected: CodeComplete request");
    } else if (RequestUID == RequestCodeCompleteClose) {
      if (!setKeyToken())
        return makeRequestInvalidError("Expected: CodeComplete token");
    }

    if (RequestUID == RequestCodeCompleteClose) {
      /// RequestCodeCompleteClose ::= { key.token: int64 }
      {
        LockGuard Guard{State->CodeCompletionCtxsLock};
        State->CodeCompletionCtxs.erase(ReqToken);
        State->CodeCompletionTokenGen.recycleToken(ReqToken);
      }
      return {};
    }

    setKeyCmdArgs();
    setKeyFiles();
    setParseParameters();
    setDiagnosticsParameters();
    setCodeCompleteParameters();

    CodeCompletionContext *Ctx;
    if (RequestUID != RequestCodeCompleteOpen) {
      LockGuard Guard{State->CodeCompletionCtxsLock};
      Ctx = State->CodeCompletionCtxs[ReqToken].get();
      if (!Ctx)
        return makeRequestInvalidError("No CodeCompletionContext");
    }

    if (RequestUID == RequestCodeCompleteOpen) {
      /// RequestCodeCompleteOpen ::= {
      ///   key.name: string,
      ///   key.line: int64,
      ///   key.column: int64,
      ///   key.sort: bool (Optional),
      ///   key.cmd.args: [string] (Optional),
      ///   key.files: [{ key.name: string, key.buffer: data }] (Optional),
      ///   key.parse.options: int64 (Optional),
      ///   key.codecomplete.options: int64 (Optional),
      ///   key.index.options : int64 (Optional),
      ///   key.index.exclude_decls_pch : bool (Optional),
      ///   key.index.diagnostics : bool (Optional),
      ///   key.diagnostics.enabled : bool (Optional),
      ///   key.diagnostics.options : int64 (Optional)
      /// }
      SafeCXIndex Idx = createSafeCXIndex();

      CXErrorCode Err;
      UniqueCXTranslationUnit &TU = Idx.parseTranslationUnit(
          nullptr, CmdArgs.data(), CmdArgs.size(), nullptr, 0, ParseOpts, Err);
      if (Err != CXError_Success)
        return makeLibclangFailedError(Err);

      // FIXME: The fact that we **have to** re-parse a TU immediately after
      // parsing it seems like a bug. However, we fail the complete-macros.c
      // test if we skip this step.
      setReparseParameters(TU);
      Err = (CXErrorCode)clang_reparseTranslationUnit(TU, 0, nullptr,
                                                      ReparseOpts);
      if (Err != CXError_Success)
        return makeLibclangFailedError(Err);

      LD.CodeCompleteResults = {clang_codeCompleteAt(
          TU, Filename.data(), LineOffset, ColOffset, UnsavedFiles.data(),
          UnsavedFiles.size(), CodeCompleteOpts)};

      auto NewCtx = make_unique<CodeCompletionContext>(std::move(Idx));
      {
        LockGuard Guard{State->CodeCompletionCtxsLock};
        ReqToken = State->CodeCompletionTokenGen.getUniqueToken();
        State->CodeCompletionCtxs[ReqToken] = std::move(NewCtx);
      }

      return toValue(LD.CodeCompleteResults, ReqToken, DoSort, IncludeBrief,
                     DoDiagnostics, DiagnosticOpts);

    } else if (RequestUID == RequestCodeComplete) {
      /// RequestCodeComplete ::= {
      ///   key.token: int64,
      ///   key.line: int64,
      ///   key.column: int64,
      ///   key.sort: bool (Optional),
      ///   key.files: [{ key.name: string, key.buffer: data }] (Optional),
      ///   key.parse.options: int64 (Optional),
      ///   key.codecomplete.options: int64 (Optional),
      ///   key.diagnostics.enabled : bool (Optional),
      ///   key.diagnostics.options : int64 (Optional)
      /// }
      LD.CodeCompleteResults = {clang_codeCompleteAt(
          Ctx->Idx.getTU(), Filename.data(), LineOffset, ColOffset,
          UnsavedFiles.data(), UnsavedFiles.size(), CodeCompleteOpts)};

      return toValue(LD.CodeCompleteResults, ReqToken, DoSort, IncludeBrief,
                     DoDiagnostics, DiagnosticOpts);

    } else if (RequestUID == RequestCodeCompleteUpdate) {
      /// RequestCodeCompleteUpdate ::= {
      ///   key.token: int64,
      ///   key.line: int64,
      ///   key.column: int64,
      ///   key.sort: bool (Optional),
      ///   key.files: [{ key.name: string, key.buffer: data }] (Optional),
      ///   key.parse.options: int64 (Optional),
      ///   key.codecomplete.options: int64 (Optional),
      ///   key.diagnostics.enabled : bool (Optional),
      ///   key.diagnostics.options : int64 (Optional)
      /// }
      setReparseParameters(Ctx->Idx.getTU());
      int Err =
          clang_reparseTranslationUnit(Ctx->Idx.getTU(), UnsavedFiles.size(),
                                       UnsavedFiles.data(), ReparseOpts);
      if (Err)
        return makeRequestFailedError("Couldn't reparse TU");

      LD.CodeCompleteResults = {clang_codeCompleteAt(
          Ctx->Idx.getTU(), Filename.data(), LineOffset, ColOffset,
          UnsavedFiles.data(), UnsavedFiles.size(), CodeCompleteOpts)};

      return toValue(LD.CodeCompleteResults, ReqToken, DoSort, IncludeBrief,
                     DoDiagnostics, DiagnosticOpts);
    }

    return makeRequestInvalidError("Unknown code complete request");
  }

  Value handleRequestIndexSource() {
    /// RequestIndexSource ::= {
    ///   key.name: string,
    ///   key.index.source.options: int64 (Optional),
    ///   key.cmd.args: [string] (Optional),
    ///   key.files: [{ key.name: string, key.buffer: data }] (Optional),
    ///   key.parse.options: int64 (Optional),
    ///   key.index.options : int64 (Optional),
    ///   key.index.exclude_decls_pch : bool (Optional),
    ///   key.index.diagnostics : bool (Optional),
    /// }
    if (!setKeyName())
      return makeRequestInvalidError("Expected: IndexSource request");

    setKeyIndexSourceOptions();
    setKeyCmdArgs();
    setKeyFiles();
    setParseParameters();

    SafeCXIndex Idx = createSafeCXIndex();
    UniqueCXIndexAction &Act = Idx.createIndexAction();

    auto NewCtx = make_unique<IndexingContext>(State, std::move(Idx));
    {
      LockGuard Guard{State->IndexingCtxsLock};
      ReqToken = State->IndexingTokenGen.getUniqueToken();
      State->IndexingCtxs[ReqToken] = std::move(NewCtx);
    }

    static IndexerCallbacks IndexerCBs = {
      // abortQuery
      handleIndexerAbortQueryEvent,

      // diagnostic
      handleIndexerDiagnosticEvent,

      // enteredMainFile
      handleIndexerEnteredMainFileEvent,

      // ppIncludedFile
      handleIndexerPPIncludeEvent,

      // importedASTFile
      handleIndexerImportedASTEvent,

      // startedTranslationUnit
      handleIndexerStartedTUEvent,

      // indexDeclaration
      handleIndexerDeclarationEvent,

      // indexEntityReference
      handleIndexerEntityReferenceEvent
    };

    CXErrorCode Err;
    Err = (CXErrorCode)clang_indexSourceFile(
        Act, (CXClientData)NewCtx.get(), &IndexerCBs, sizeof(IndexerCBs),
        IndexSourceOpts, Filename.data(), CmdArgs.data(), CmdArgs.size(),
        UnsavedFiles.data(), UnsavedFiles.size(), nullptr, ParseOpts);
    if (Err != CXError_Success)
      return makeLibclangFailedError(Err);

    return makeRequestFailedError("Indexing not supported");
  }

public:
  ResponseBuilder(ServiceState *State, LiveData &LD, Value::Dict &ClientDict,
                  StringRef RequestStr, CSUID RequestUID)
      : State(State), LD(LD), ClientDict(ClientDict), RequestStr(RequestStr),
        RequestUID(RequestUID) {}

  Value build() {
    if (RequestUID == RequestVersion)
      return handleRequestVersion();

    if (RequestStr.startswith(RequestCodeComplete.getName()))
      return handleRequestCodeComplete();

    if (RequestUID == RequestIndexSource)
      return handleRequestIndexSource();

    return makeRequestInvalidError("Unknown request");
  }
};

Value Service::handle(Value Request, LiveData &LD) {
  log("Handling incoming request:");
  log(Request);

  if (!Request.isDict())
    return makeRequestInvalidError("!Request.isDict()");

  Value::Dict &ClientDict = Request.getDict();
  Value &ClientKeyRequest = ClientDict[KeyRequest];
  if (!ClientKeyRequest.isStringLike())
    return makeRequestInvalidError("!ClientKeyRequest.isStringLike()");

  StringRef RequestStr = ClientKeyRequest.getStringRef();
  if (!CSUID::isValidIdentifier(RequestStr))
    return makeRequestInvalidError("!isValidIdentifier(RequestStr)");

  CSUID RequestUID{RequestStr};

  ResponseBuilder RB{State, LD, ClientDict, RequestStr, RequestUID};
  return RB.build();
}

ErrorKind Service::getResponseErrorKind(Value &Response) {
  if (!Response.isDataLike())
    return ErrorKind::NoError;

  auto EK = ErrorKind(Response.getData()[0]);
  if (EK == ErrorKind::RequestInvalid) return EK;
  if (EK == ErrorKind::RequestFailed) return EK;
  if (EK == ErrorKind::RequestInterrupted) return EK;
  if (EK == ErrorKind::RequestCancelled) return EK;
  if (EK == ErrorKind::Fatal) return EK;
  return ErrorKind::NoError;
}

StringRef Service::getResponseErrorString(Value &Response) {
  if (getResponseErrorKind(Response) == ErrorKind::NoError)
    return "NoError";
  return Response.getDataRef().substr(1);
}

void Service::log(Value &V) {
  LockGuard Guard{State->LogLock};
  std::ofstream LogFile("/tmp/ClangXPC.log",
                        std::ofstream::out | std::ofstream::app);
  llvm::raw_os_ostream Log(LogFile);
  Log << "[ClangService] Value.structure: ";
  V.structure(Log);
  Log << "[ClangService] Value.dump     : ";
  V.dump(Log);
  Log.flush();
  LogFile.close();
}

void Service::log(StringRef Msg) {
  LockGuard Guard{State->LogLock};
  std::ofstream LogFile("/tmp/ClangXPC.log",
                        std::ofstream::out | std::ofstream::app);
  llvm::raw_os_ostream Log(LogFile);
  Log << "[ClangService] " << Msg << '\n';
  Log.flush();
  LogFile.close();
}

} // end namespace ClangService
