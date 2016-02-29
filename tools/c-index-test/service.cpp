#include "service.h"

#include "clang-service/Service.h"
#include "clang-service/SafeLibclang.h"
#include "clang-service/Support/InProcessClient.h"
#include "clang-service/Support/Value.h"
#include "clang-service/Support/ValueUtils.h"

#include "llvm/ADT/STLExtras.h"

using namespace ClangService;
using namespace Libclang;

namespace {
#define REGISTER_UID(Name, UIDStr) LazyCSUID Name{UIDStr};
#include "clang-service/ProtocolUIDs.inc"
}

CUniqueStr wrapString(Value &V) {
  if (V.isNull())
    return CUniqueStr(strdup(""));

  auto SR = V.getStringRef();
  return CUniqueStr(strndup(SR.data(), SR.size()));
}

CUniqueStr wrapString(UniqueCXString &S) {
  const char *str = clang_getCString(S) ? clang_getCString(S) : "";
  return CUniqueStr(strdup(str));
}

struct CSCXFile {
  bool IsCS; ///< Is this a ClangService value?
  Value *CS; ///< A ClangService file, or null.
  CXFile CX; ///< A libclang file, or null.

  CSCXFile(Value &V) : IsCS(true), CS(&V) {}

  CSCXFile(CXFile V) : IsCS(false), CX(V) {}

  operator bool() {
    if (IsCS) {
      return CS->getDict()[KeyLocationFileUID].isDataLike();
    } else {
      return CX;
    }
  }

  friend bool operator==(CSCXFile &L, CSCXFile &R) {
    assert(L.IsCS == R.IsCS);
    if (L.IsCS) {
      return L.CS->getDict()[KeyLocationFileUID] ==
             R.CS->getDict()[KeyLocationFileUID];
    } else {
      return L.CX == R.CX;
    }
  }
};

struct CSCXSourceLocation {
  bool IsCS; ///< Is this a ClangService value?
  Value *CS; ///< A ClangService source location, or null.
  CXSourceLocation CX; ///< A libclang source location, or null.

  CSCXSourceLocation(Value &V) : IsCS(true), CS(&V) {}

  CSCXSourceLocation(CXSourceLocation V) : IsCS(false), CX(V) {}

  CSCXFile getSpellingLocation(unsigned *line, unsigned *column,
                               unsigned *offset) {
    if (IsCS) {
      if (line)
        *line =
            (unsigned)CS->getDict()[KeyLocationSpellingLineOffset].getInt64();
      if (column)
        *column =
            (unsigned)CS->getDict()[KeyLocationSpellingColOffset].getInt64();
      if (offset)
        *offset = (unsigned)CS->getDict()[KeyLocationSpellingOffset].getInt64();
      return {CS->getDict()[KeyLocationSpellingFile]};
    } else {
      CXFile file;
      clang_getSpellingLocation(CX, &file, line, column, offset);
      return {file};
    }
  }

  friend bool operator==(CSCXSourceLocation &L, CSCXSourceLocation &R) {
    assert(L.IsCS == R.IsCS);
    if (L.IsCS) {
      unsigned l_line, l_col, l_off;
      unsigned r_line, r_col, r_off;
      CSCXFile l_file = {L.getSpellingLocation(&l_line, &l_col, &l_off)};
      CSCXFile r_file = {R.getSpellingLocation(&r_line, &r_col, &r_off)};
      return l_line == r_line && l_col == r_col && l_off == r_off &&
             l_file == r_file;
    } else {
      return clang_equalLocations(L.CX, R.CX);
    }
  }
};

struct CSCXSourceRange {
  bool IsCS; ///< Is this a ClangService value?
  Value *CS; ///< A ClangService source range, or null.
  CXSourceRange CX; ///< A libclang source range, or null.

  CSCXSourceRange() : IsCS(false), CS(nullptr), CX() {}

  CSCXSourceRange(Value &V) : IsCS(true), CS(&V) {}

  CSCXSourceRange(CXSourceRange V) : IsCS(false), CX(V) {}

  CSCXSourceLocation getStart() {
    if (IsCS) {
      return {CS->getDict()[KeyRangeStart]};
    } else {
      return {clang_getRangeStart(CX)};
    }
  }

  CSCXSourceLocation getEnd() {
    if (IsCS) {
      return {CS->getDict()[KeyRangeEnd]};
    } else {
      return {clang_getRangeEnd(CX)};
    }
  }
};

struct CSCXDiagnostic {
  bool IsCS; ///< Is this a ClangService value?
  Value *CS; ///< A ClangService diagnostic, or null.
  CXDiagnostic CX; ///< A libclang diagnostic, or null.

  CSCXDiagnostic(Value &V) : IsCS(true), CS(&V) {}

  CSCXDiagnostic(CXDiagnostic V) : IsCS(false), CX(V) {}

  CXDiagnosticSeverity getSeverity() {
    if (IsCS) {
      return (CXDiagnosticSeverity)CS->getDict()[KeyDiagnosticSeverity]
          .getInt64();
    } else {
      return clang_getDiagnosticSeverity(CX);
    }
  }

  CUniqueStr getFormattedStr(unsigned display_opts) {
    if (IsCS) {
      return wrapString(CS->getDict()[KeyDiagnosticString]);
    } else {
      UniqueCXString diag{clang_formatDiagnostic(CX, display_opts)};
      return wrapString(diag);
    }
  }

  CSCXSourceLocation getLocation() {
    if (IsCS) {
      return {CS->getDict()[KeyDiagnosticLocation]};
    } else {
      return {clang_getDiagnosticLocation(CX)};
    }
  }

  unsigned getNumFixIts() {
    if (IsCS) {
      return (unsigned)CS->getDict()[KeyDiagnosticFixits].getArray().size();
    } else {
      return clang_getDiagnosticNumFixIts(CX);
    }
  }

  CUniqueStr getFixIt(unsigned I, CSCXSourceRange *range) {
    if (IsCS) {
      auto &Fixit = CS->getDict()[KeyDiagnosticFixits].getArray()[I].getArray();
      *range = {Fixit[1]};
      return wrapString(Fixit[0]);
    } else {
      CXSourceRange cx_range;
      UniqueCXString fixit{clang_getDiagnosticFixIt(CX, I, &cx_range)};
      *range = {cx_range};
      return wrapString(fixit);
    }
  }
};

struct CSCXCompletionString {
  bool IsCS; ///< Is this a ClangService value?
  Value *CS; ///< A ClangService completion string, or null.
  CXCompletionString CX; ///< A libclang completion string, or null.

  CSCXCompletionString(Value &V) : IsCS(true), CS(&V) {}

  CSCXCompletionString(CXCompletionString V) : IsCS(false), CX(V) {}

  Value::Array &getChunksArray() {
    assert(IsCS);
    return CS->getDict()[KeyCodeCompleteCompletionStrChunks].getArray();
  }

  unsigned getNumChunks() {
    if (IsCS)
      return getChunksArray().size();
    else
      return clang_getNumCompletionChunks(CX);
  }

  CXCompletionChunkKind getChunkKind(unsigned I) {
    if (IsCS)
      return (CXCompletionChunkKind)getChunksArray()[I]
          .getDict()[KeyCodeCompleteCompletionStrKind]
          .getInt64();
    else
      return clang_getCompletionChunkKind(CX, I);
  }

  CSCXCompletionString getChunk(unsigned I) {
    if (IsCS)
      return {getChunksArray()[I]};
    else
      return {clang_getCompletionChunkCompletionString(CX, I)};
  }

  CUniqueStr getChunkText(unsigned I) {
    if (IsCS) {
      return wrapString(
          getChunksArray()[I].getDict()[KeyCodeCompleteCompletionStrText]);
    } else {
      UniqueCXString text{clang_getCompletionChunkText(CX, I)};
      return wrapString(text);
    }
  }

  unsigned getPriority() {
    if (IsCS)
      return CS->getDict()[KeyCodeCompleteCompletionStrPriority].getInt64();
    else
      return clang_getCompletionPriority(CX);
  }

  CXAvailabilityKind getAvailability() {
    if (IsCS)
      return (CXAvailabilityKind)CS
          ->getDict()[KeyCodeCompleteCompletionStrAvailability]
          .getInt64();
    else
      return clang_getCompletionAvailability(CX);
  }

  unsigned getNumAnnotations() {
    if (IsCS)
      return CS->getDict()[KeyCodeCompleteCompletionStrAnnotations]
          .getArray()
          .size();
    else
      return clang_getCompletionNumAnnotations(CX);
  }

  CUniqueStr getAnnotation(unsigned I) {
    if (IsCS) {
      return wrapString(
          CS->getDict()[KeyCodeCompleteCompletionStrAnnotations].getArray()[I]);
    } else {
      UniqueCXString annotation{clang_getCompletionAnnotation(CX, I)};
      return wrapString(annotation);
    }
  }

  CUniqueStr getParent(CXCursorKind *ParentKind) {
    if (IsCS) {
      *ParentKind =
          (CXCursorKind)CS->getDict()[KeyCodeCompleteCompletionStrParentKind]
              .getInt64();
      return wrapString(CS->getDict()[KeyCodeCompleteCompletionStrParent]);
    } else {
      UniqueCXString parent{clang_getCompletionParent(CX, ParentKind)};
      return wrapString(parent);
    }
  }

  CUniqueStr getBriefComment() {
    if (IsCS) {
      return wrapString(CS->getDict()[KeyCodeCompleteCompletionStrBrief]);
    } else {
      UniqueCXString brief{clang_getCompletionBriefComment(CX)};
      return wrapString(brief);
    }
  }

  void print(FILE *file) {
    unsigned N = getNumChunks();
    for (unsigned I = 0; I != N; ++I) {
      CXCompletionChunkKind Kind = getChunkKind(I);

      if (Kind == CXCompletionChunk_Optional) {
        fprintf(file, "{Optional ");
        getChunk(I).print(file);
        fprintf(file, "}");
        continue;
      }

      if (Kind == CXCompletionChunk_VerticalSpace) {
        fprintf(file, "{VerticalSpace  }");
        continue;
      }

      CUniqueStr cstr = getChunkText(I);
      fprintf(file, "{%s %s}", clang_getCompletionChunkKindSpelling(Kind),
              cstr ? cstr.get() : "");
    }
  }
};

struct CSCXCompletionResult {
  bool IsCS; ///< Is this a ClangService value?
  Value *CS; ///< A ClangService completion result, or null.
  CXCompletionResult *CX; ///< A libclang completion result, or null.

  CSCXCompletionResult(Value &V) : IsCS(true), CS(&V) {}

  CSCXCompletionResult(CXCompletionResult *V) : IsCS(false), CX(V) {}

  CXCursorKind getCursorKind() {
    if (IsCS)
      return (CXCursorKind)CS->getDict()[KeyCodeCompleteCompletionKind]
          .getInt64();
    else
      return CX->CursorKind;
  }

  CSCXCompletionString getCompletionString() {
    if (IsCS)
      return {CS->getDict()[KeyCodeCompleteCompletionStr]};
    else
      return {CX->CompletionString};
  }

  void print(FILE *file) {
    UniqueCXString ks{clang_getCursorKindSpelling(getCursorKind())};
    fprintf(file, "%s:", clang_getCString(ks));

    CSCXCompletionString CompletionString = getCompletionString();
    CompletionString.print(file);
    fprintf(file, " (%u)", CompletionString.getPriority());
    switch (CompletionString.getAvailability()) {
    case CXAvailability_Available:
      break;

    case CXAvailability_Deprecated:
      fprintf(file, " (deprecated)");
      break;

    case CXAvailability_NotAvailable:
      fprintf(file, " (unavailable)");
      break;

    case CXAvailability_NotAccessible:
      fprintf(file, " (inaccessible)");
      break;
    }

    unsigned annotationCount = CompletionString.getNumAnnotations();
    if (annotationCount) {
      unsigned i;
      fprintf(file, " (");
      for (i = 0; i < annotationCount; ++i) {
        if (i != 0)
          fprintf(file, ", ");
        fprintf(file, "\"%s\"", CompletionString.getAnnotation(i).get());
      }
      fprintf(file, ")");
    }

    if (!getenv("CINDEXTEST_NO_COMPLETION_PARENTS")) {
      CXCursorKind ParentKind;
      CUniqueStr ParentName = CompletionString.getParent(&ParentKind);
      if (ParentKind != CXCursor_NotImplemented) {
        UniqueCXString KindSpelling{clang_getCursorKindSpelling(ParentKind)};
        fprintf(file, " (parent: %s '%s')", clang_getCString(KindSpelling),
                ParentName.get());
      }
    }

    const char *BriefCommentCString = CompletionString.getBriefComment().get();
    if (BriefCommentCString && *BriefCommentCString != '\0') {
      fprintf(file, "(brief comment: %s)", BriefCommentCString);
    }

    fprintf(file, "\n");
  }
};

extern "C" {

// FIXME: Dead code, for debugging only.
void read_file_until_cur(FILE *file, char **out_buf, off_t *out_buf_len) {
  off_t bytes_read, extra_bytes_read;

  *out_buf_len = ftello(file);
  *out_buf = (char *)malloc(*out_buf_len + 1);
  if (!*out_buf)
    return;

  rewind(file);

  bytes_read = 0;
  while (bytes_read < *out_buf_len) {
    extra_bytes_read =
        fread(*out_buf + bytes_read, 1, *out_buf_len - bytes_read, file);
    if (extra_bytes_read == 0) {
      perror("fread");
      free(*out_buf);
      *out_buf = NULL;
      return;
    }
    bytes_read += extra_bytes_read;
  }

  (*out_buf)[*out_buf_len] = '\0';
}

void PrintExtent(FILE *out, unsigned begin_line, unsigned begin_column,
                        unsigned end_line, unsigned end_column) {
  fprintf(out, "[%d:%d - %d:%d]", begin_line, begin_column,
          end_line, end_column);
}

void PrintDiagnosticImpl(CSCXDiagnostic &Diagnostic, FILE *out) {
  if (Diagnostic.getSeverity() == CXDiagnostic_Ignored)
    return;

  unsigned display_opts =
      CXDiagnostic_DisplaySourceLocation | CXDiagnostic_DisplayColumn |
      CXDiagnostic_DisplaySourceRanges | CXDiagnostic_DisplayOption;

  fprintf(out, "%s\n", Diagnostic.getFormattedStr(display_opts).get());

  CSCXSourceLocation loc = Diagnostic.getLocation();
  CSCXFile file = loc.getSpellingLocation(nullptr, nullptr, nullptr);
  if (!file)
    return;

  unsigned num_fixits = Diagnostic.getNumFixIts();
  fprintf(stderr, "Number FIX-ITs = %d\n", (int) num_fixits);

  for (unsigned i = 0; i != num_fixits; ++i) {
    CSCXSourceRange range;
    char *insertion_text = Diagnostic.getFixIt(i, &range).get();
    CSCXSourceLocation start = range.getStart();
    CSCXSourceLocation end = range.getEnd();
    unsigned start_line, start_column, end_line, end_column;
    CSCXFile start_file =
        start.getSpellingLocation(&start_line, &start_column, nullptr);
    CSCXFile end_file =
        end.getSpellingLocation(&end_line, &end_column, nullptr);
    if (start == end) {
      /* Insertion. */
      if (start_file == file)
        fprintf(out, "FIX-IT: Insert \"%s\" at %d:%d\n",
                insertion_text, start_line, start_column);
    } else if (strcmp(insertion_text, "") == 0) {
      /* Removal. */
      if (start_file == file && end_file == file) {
        fprintf(out, "FIX-IT: Remove ");
        PrintExtent(out, start_line, start_column, end_line, end_column);
        fprintf(out, "\n");
      }
    } else {
      /* Replacement. */
      if (start_file == end_file) {
        fprintf(out, "FIX-IT: Replace ");
        PrintExtent(out, start_line, start_column, end_line, end_column);
        fprintf(out, " with \"%s\"\n", insertion_text);
      }
    }
  }
}

void PrintDiagnostic(CXDiagnostic Diagnostic) {
  CSCXDiagnostic diagnostic_wrapper{Diagnostic};
  PrintDiagnosticImpl(diagnostic_wrapper, stderr);
}

void print_completion_string(CXCompletionString completion_string, FILE *file) {
  CSCXCompletionString completion_string_wrapper{completion_string};
  completion_string_wrapper.print(file);
}

void print_completion_result(CXCompletionResult *completion_result,
                             FILE *file) {
  CSCXCompletionResult completion_result_wrapper{completion_result};
  completion_result_wrapper.print(file);
}

void print_completion_contexts(unsigned long long contexts, FILE *file) {
  fprintf(file, "Completion contexts:\n");
  if (contexts == CXCompletionContext_Unknown) {
    fprintf(file, "Unknown\n");
  }
  if (contexts & CXCompletionContext_AnyType) {
    fprintf(file, "Any type\n");
  }
  if (contexts & CXCompletionContext_AnyValue) {
    fprintf(file, "Any value\n");
  }
  if (contexts & CXCompletionContext_ObjCObjectValue) {
    fprintf(file, "Objective-C object value\n");
  }
  if (contexts & CXCompletionContext_ObjCSelectorValue) {
    fprintf(file, "Objective-C selector value\n");
  }
  if (contexts & CXCompletionContext_CXXClassTypeValue) {
    fprintf(file, "C++ class type value\n");
  }
  if (contexts & CXCompletionContext_DotMemberAccess) {
    fprintf(file, "Dot member access\n");
  }
  if (contexts & CXCompletionContext_ArrowMemberAccess) {
    fprintf(file, "Arrow member access\n");
  }
  if (contexts & CXCompletionContext_ObjCPropertyAccess) {
    fprintf(file, "Objective-C property access\n");
  }
  if (contexts & CXCompletionContext_EnumTag) {
    fprintf(file, "Enum tag\n");
  }
  if (contexts & CXCompletionContext_UnionTag) {
    fprintf(file, "Union tag\n");
  }
  if (contexts & CXCompletionContext_StructTag) {
    fprintf(file, "Struct tag\n");
  }
  if (contexts & CXCompletionContext_ClassTag) {
    fprintf(file, "Class name\n");
  }
  if (contexts & CXCompletionContext_Namespace) {
    fprintf(file, "Namespace or namespace alias\n");
  }
  if (contexts & CXCompletionContext_NestedNameSpecifier) {
    fprintf(file, "Nested name specifier\n");
  }
  if (contexts & CXCompletionContext_ObjCInterface) {
    fprintf(file, "Objective-C interface\n");
  }
  if (contexts & CXCompletionContext_ObjCProtocol) {
    fprintf(file, "Objective-C protocol\n");
  }
  if (contexts & CXCompletionContext_ObjCCategory) {
    fprintf(file, "Objective-C category\n");
  }
  if (contexts & CXCompletionContext_ObjCInstanceMessage) {
    fprintf(file, "Objective-C instance method\n");
  }
  if (contexts & CXCompletionContext_ObjCClassMessage) {
    fprintf(file, "Objective-C class method\n");
  }
  if (contexts & CXCompletionContext_ObjCSelectorName) {
    fprintf(file, "Objective-C selector name\n");
  }
  if (contexts & CXCompletionContext_MacroName) {
    fprintf(file, "Macro name\n");
  }
  if (contexts & CXCompletionContext_NaturalLanguage) {
    fprintf(file, "Natural language\n");
  }
}

void print_container_kind_impl(CXCursorKind containerKind,
                               int containerIsIncomplete, CUniqueStr USR,
                               CUniqueStr selectorString, FILE *file) {
  if (containerKind == CXCursor_InvalidCode)
    return;

  /* We have found a container */
  UniqueCXString containerKindSpelling{
      clang_getCursorKindSpelling(containerKind)};
  fprintf(file, "Container Kind: %s\n",
          clang_getCString(containerKindSpelling));

  if (containerIsIncomplete) {
    fprintf(file, "Container is incomplete\n");
  } else {
    fprintf(file, "Container is complete\n");
  }

  fprintf(file, "Container USR: %s\n", USR.get());

  if (strlen(selectorString.get()) > 0)
    fprintf(file, "Objective-C selector: %s\n", selectorString.get());
}

void print_container_kind(CXCursorKind containerKind,
                          int containerIsIncomplete,
                          CXCodeCompleteResults *results, FILE *file) {
  UniqueCXString containerUSR{clang_codeCompleteGetContainerUSR(results)};
  UniqueCXString objCSelector{clang_codeCompleteGetObjCSelector(results)};
  print_container_kind_impl(containerKind, containerIsIncomplete,
                            wrapString(containerUSR), wrapString(objCSelector),
                            file);
}

const char *
clang_getCompletionChunkKindSpelling(CXCompletionChunkKind Kind) {
  switch (Kind) {
  case CXCompletionChunk_Optional:
    return "Optional";
  case CXCompletionChunk_TypedText:
    return "TypedText";
  case CXCompletionChunk_Text:
    return "Text";
  case CXCompletionChunk_Placeholder:
    return "Placeholder";
  case CXCompletionChunk_Informative:
    return "Informative";
  case CXCompletionChunk_CurrentParameter:
    return "CurrentParameter";
  case CXCompletionChunk_LeftParen:
    return "LeftParen";
  case CXCompletionChunk_RightParen:
    return "RightParen";
  case CXCompletionChunk_LeftBracket:
    return "LeftBracket";
  case CXCompletionChunk_RightBracket:
    return "RightBracket";
  case CXCompletionChunk_LeftBrace:
    return "LeftBrace";
  case CXCompletionChunk_RightBrace:
    return "RightBrace";
  case CXCompletionChunk_LeftAngle:
    return "LeftAngle";
  case CXCompletionChunk_RightAngle:
    return "RightAngle";
  case CXCompletionChunk_Comma:
    return "Comma";
  case CXCompletionChunk_ResultType:
    return "ResultType";
  case CXCompletionChunk_Colon:
    return "Colon";
  case CXCompletionChunk_SemiColon:
    return "SemiColon";
  case CXCompletionChunk_Equal:
    return "Equal";
  case CXCompletionChunk_HorizontalSpace:
    return "HorizontalSpace";
  case CXCompletionChunk_VerticalSpace:
    return "VerticalSpace";
  }

  return "Unknown";
}

int clangservice_perform_code_completion(
    const char *source_filename, unsigned complete_line,
    unsigned complete_column, const char *const *command_line_args,
    int num_command_line_args, struct CXUnsavedFile *unsaved_files,
    unsigned num_unsaved_files, unsigned parsing_options,
    unsigned completion_options, int timing_only, FILE *outfile) {
  auto C = llvm::make_unique<InProcessClient>();

  unsigned display_opts =
      CXDiagnostic_DisplaySourceLocation | CXDiagnostic_DisplayColumn |
      CXDiagnostic_DisplaySourceRanges | CXDiagnostic_DisplayOption;

  Value OpenReq = Value::dict(
      {{KeyRequest, RequestCodeCompleteOpen.c_str()},
       {KeyName, source_filename},
       {KeyLineOffset, int64_t(complete_line)},
       {KeyColOffset, int64_t(complete_column)},
       {KeyCmdArgs, toStringArray(llvm::ArrayRef<const char *>(
                        command_line_args, num_command_line_args))},
       {KeyFiles, toUnsavedFilesArray(llvm::ArrayRef<CXUnsavedFile *>(
                      &unsaved_files, num_unsaved_files))},
       {KeySort, !timing_only},
       {KeyParseOptions, int64_t(parsing_options)},
       {KeyCodeCompleteOptions, int64_t(completion_options)},
       {KeyDiagnosticsEnabled, true},
       {KeyDiagnosticsOptions, int64_t(display_opts)}});

  Value OpenResp = C->request(std::move(OpenReq));
  if (Service::getResponseErrorKind(OpenResp) != ErrorKind::NoError) {
    fprintf(stderr, "ClangService error: %s\n",
            Service::getResponseErrorString(OpenResp).data());
    return -1;
  }

  Value::Dict &RespDict = OpenResp.getDict();

  int64_t Token = RespDict[KeyToken].getInt64();

  Value::Array &Results = RespDict[KeyCodeCompleteResults].getArray();
  if (!timing_only)
    for (Value &Result : Results)
      CSCXCompletionResult(Result).print(outfile);

  if (RespDict[KeyCodeCompleteDiagnostics].isArray()) {
    auto &ADiagnostics = RespDict[KeyCodeCompleteDiagnostics].getArray();
    for (auto &Diagnostic : ADiagnostics) {
      CSCXDiagnostic Diag{Diagnostic};
      PrintDiagnosticImpl(Diag, stderr);
    }
  }

  print_completion_contexts(
      (unsigned long long)RespDict[KeyCodeCompleteContexts].getInt64(),
      outfile);

  auto CK = (CXCursorKind)RespDict[KeyCodeCompleteContainerKind].getInt64();
  print_container_kind_impl(
      CK, RespDict[KeyCodeCompleteContainerIncomplete].getBool(),
      wrapString(RespDict[KeyCodeCompleteContainerUSR]),
      wrapString(RespDict[KeyCodeCompleteObjCSelector]), outfile);

  Value CloseResp = C->request(Value::dict(
      {{KeyRequest, RequestCodeCompleteClose.c_str()}, {KeyToken, Token}}));

  return CloseResp.isNull() ? 0 : -1;
}

} // end extern "C"
