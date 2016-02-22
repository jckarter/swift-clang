//===--- IndexRecordWriter.cpp - Index record serialization ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "IndexRecordWriter.h"
#include "FileIndexRecord.h"
#include "IndexDataStoreUtils.h"
#include "clang/Index/IndexSymbol.h"
#include "clang/Index/IndexRecordReader.h"
#include "clang/Index/USRGeneration.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclObjC.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Bitcode/BitstreamWriter.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <unistd.h>

using namespace clang;
using namespace clang::index;
using namespace clang::index::store;
using namespace llvm;

StringRef IndexRecordWriter::getUSR(const Decl *D) {
  assert(D->isCanonicalDecl());
  auto Insert = USRByDecl.insert(std::make_pair(D, StringRef()));
  if (Insert.second) {
    Insert.first->second = getUSRNonCached(D);
  }
  return Insert.first->second;
}

StringRef IndexRecordWriter::getUSRNonCached(const Decl *D) {
  SmallString<256> Buf;
  bool Ignore = generateUSRForDecl(D, Buf);
  if (Ignore)
    return StringRef();
  StringRef USR = Buf.str();
  char *Ptr = Allocator.Allocate<char>(USR.size());
  std::copy(USR.begin(), USR.end(), Ptr);
  return StringRef(Ptr, USR.size());
}

IndexRecordWriter::IndexRecordWriter(ASTContext &Ctx, RecordingOptions Opts)
  : Ctx(Ctx),
    RecordOpts(std::move(Opts)),
    Hasher(Ctx) {
  RecordsPath = RecordOpts.DataDirPath;
  store::makeRecordSubDir(RecordsPath);
  if (Opts.RecordSymbolCodeGenName)
    CGNameGen.reset(new CodegenNameGenerator(Ctx));
}

IndexRecordWriter::~IndexRecordWriter() {}

static void writeBlockInfo(BitstreamWriter &Stream);
static void writeVersionInfo(BitstreamWriter &Stream);
static void writeDecls(BitstreamWriter &Stream, const FileIndexRecord &IdxRecord,
                       IndexRecordWriter &Writer);

namespace {
struct DeclInfo {
  const Decl *D;
  SymbolRoleSet Roles;
  SymbolRoleSet RelatedRoles;
  DeclInfo(const Decl *D, SymbolRoleSet Roles, SymbolRoleSet RelatedRoles)
    : D(D), Roles(Roles), RelatedRoles(RelatedRoles) {}
};

struct OccurDeclID {
  unsigned DeclID;
  SmallVector<unsigned, 4> RelatedDeclIDs;
};
}

static void extractInfoFromRecord(const FileIndexRecord &IdxRecord,
                                  SmallVectorImpl<DeclInfo> &Decls,
                                  SmallVectorImpl<OccurDeclID> &OccurDeclIDs) {
  DenseMap<const Decl *, unsigned> IndexForDecl;

  // Pairs of (decl ID, parent ID).
  OccurDeclIDs.reserve(IdxRecord.getDeclOccurrences().size());

  auto insertDecl = [&](const Decl *D,
                        SymbolRoleSet Roles,
                        SymbolRoleSet RelatedRoles) -> unsigned {
    DenseMap<const Decl *, unsigned>::iterator It;
    bool IsNew;
    std::tie(It, IsNew) = IndexForDecl.insert(std::make_pair(D, Decls.size()));
    if (IsNew) {
      Decls.emplace_back(D, Roles, RelatedRoles);
    } else {
      Decls[It->second].Roles |= Roles;
      Decls[It->second].RelatedRoles |= RelatedRoles;
    }
    return It->second+1;
  };

  for (auto &Occur : IdxRecord.getDeclOccurrences()) {
    unsigned DeclID = insertDecl(Occur.Dcl, Occur.Roles, SymbolRoleSet());

    SmallVector<unsigned, 4> RelatedDeclIDs;
    RelatedDeclIDs.reserve(Occur.Relations.size());
    for (auto &Rel : Occur.Relations) {
      unsigned ID = insertDecl(Rel.RelatedSymbol, SymbolRoleSet(), Rel.Roles);
      RelatedDeclIDs.push_back(ID);
    }
    OccurDeclID OccDID{ DeclID, std::move(RelatedDeclIDs) };
    OccurDeclIDs.push_back(std::move(OccDID));
  }
}

bool IndexRecordWriter::writeRecord(StringRef Filename,
                                    const FileIndexRecord &IdxRecord,
                                    std::string &Error,
                                    std::string *OutRecordFile) {
  using namespace llvm::sys;
  std::string HashString = APInt(64, Hasher.hashRecord(IdxRecord)).toString(36, /*Signed=*/false);

  SmallString<256> RecordPath = StringRef(RecordsPath);
  RecordPath += '/';
  RecordPath += path::filename(Filename);
  RecordPath += '-';
  RecordPath += HashString;
  if (OutRecordFile)
    *OutRecordFile = path::filename(RecordPath);

  std::error_code EC;
  raw_fd_ostream OS(RecordPath.str(), EC, sys::fs::F_Excl);
  if (EC) {
    if (EC == errc::file_exists) {
      // Same header contents were written out by another compiler invocation.
      return false;
    }
    Error = EC.message();
    return true;
  }

  SmallString<512> Buffer;
  BitstreamWriter Stream(Buffer);
  Stream.Emit('I', 8);
  Stream.Emit('D', 8);
  Stream.Emit('X', 8);
  Stream.Emit('S', 8);

  writeBlockInfo(Stream);
  writeVersionInfo(Stream);

  if (!IdxRecord.getDeclOccurrences().empty()) {
    writeDecls(Stream, IdxRecord, *this);
  }

  OS.write(Buffer.data(), Buffer.size());
  OS.close();

  return false;
}

static void EmitBlockID(unsigned ID, const char *Name,
                        llvm::BitstreamWriter &Stream,
                        RecordDataImpl &Record) {
  Record.clear();
  Record.push_back(ID);
  Stream.EmitRecord(llvm::bitc::BLOCKINFO_CODE_SETBID, Record);

  // Emit the block name if present.
  if (!Name || Name[0] == 0)
    return;
  Record.clear();
  while (*Name)
    Record.push_back(*Name++);
  Stream.EmitRecord(llvm::bitc::BLOCKINFO_CODE_BLOCKNAME, Record);
}

static void EmitRecordID(unsigned ID, const char *Name,
                         llvm::BitstreamWriter &Stream,
                         RecordDataImpl &Record) {
  Record.clear();
  Record.push_back(ID);
  while (*Name)
    Record.push_back(*Name++);
  Stream.EmitRecord(llvm::bitc::BLOCKINFO_CODE_SETRECORDNAME, Record);
}

static void writeBlockInfo(BitstreamWriter &Stream) {
  RecordData Record;

  Stream.EnterBlockInfoBlock(3);
#define BLOCK(X) EmitBlockID(X ## _ID, #X, Stream, Record)
#define RECORD(X) EmitRecordID(X, #X, Stream, Record)

  BLOCK(VERSION_BLOCK);
  RECORD(REC_VERSION);

  BLOCK(DECLS_BLOCK);
  RECORD(REC_DECLINFO);

  BLOCK(DECLOFFSETS_BLOCK);
  RECORD(REC_DECLOFFSETS);

  BLOCK(DECLOCCURRENCES_BLOCK);
  RECORD(REC_DECLOCCURRENCE);

#undef RECORD
#undef BLOCK
  Stream.ExitBlock();
}

static void writeVersionInfo(BitstreamWriter &Stream) {
  using namespace llvm::sys;

  Stream.EnterSubblock(VERSION_BLOCK_ID, 3);

  BitCodeAbbrev *Abbrev = new BitCodeAbbrev();
  Abbrev->Add(BitCodeAbbrevOp(REC_VERSION));
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 6)); // Store format version
  unsigned AbbrevCode = Stream.EmitAbbrev(Abbrev);

  RecordData Record;
  Record.push_back(REC_VERSION);
  Record.push_back(STORE_FORMAT_VERSION);
  Stream.EmitRecordWithAbbrev(AbbrevCode, Record);

  Stream.ExitBlock();
}

template <typename T, typename Allocator>
static StringRef data(const std::vector<T, Allocator> &v) {
  if (v.empty()) return StringRef();
  return StringRef(reinterpret_cast<const char*>(&v[0]),
                   sizeof(T) * v.size());
}

template <typename T>
static StringRef data(const SmallVectorImpl<T> &v) {
  return StringRef(reinterpret_cast<const char*>(v.data()),
                   sizeof(T) * v.size());
}

static void writeDecls(BitstreamWriter &Stream, const FileIndexRecord &IdxRecord,
                       IndexRecordWriter &Writer) {
  SmallVector<DeclInfo, 32> Decls;
  SmallVector<OccurDeclID, 64> OccurDeclIDs;
  extractInfoFromRecord(IdxRecord, Decls, OccurDeclIDs);

  SmallVector<uint32_t, 32> DeclOffsets;
  DeclOffsets.reserve(Decls.size());

  ASTContext &Ctx = Writer.getASTContext();
  SourceManager &SM = Ctx.getSourceManager();

  //===--------------------------------------------------------------------===//
  // DECLS_BLOCK_ID
  //===--------------------------------------------------------------------===//

  Stream.EnterSubblock(DECLS_BLOCK_ID, 3);

  BitCodeAbbrev *Abbrev = new BitCodeAbbrev();
  Abbrev->Add(BitCodeAbbrevOp(REC_DECLINFO));
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 5)); // Kind
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 2)); // Language
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 2)); // C++ template kind
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, SymbolRoleBitNum)); // Roles
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, SymbolRoleBitNum)); // Related Roles
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 6)); // Length of name in block
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 6)); // Length of USR in block
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Blob)); // Name + USR + CodeGen symbol name
  unsigned AbbrevCode = Stream.EmitAbbrev(Abbrev);

  RecordData Record;

#ifndef NDEBUG
  StringMap<PresumedLoc> USRSet;
#endif

  PrintingPolicy Policy(Ctx.getLangOpts());
  Policy.SuppressTemplateArgsInCXXConstructors = true;

  CodegenNameGenerator *CGNameGen = Writer.getCGNameGen();

  llvm::SmallString<256> Blob;
  for (auto &Info : Decls) {
    DeclOffsets.push_back(Stream.GetCurrentBitNo());

    Blob.clear();
    auto ND = dyn_cast<NamedDecl>(Info.D);
    if (ND) {
      llvm::raw_svector_ostream OS(Blob);
      DeclarationName DeclName = ND->getDeclName();
      if (!DeclName.isEmpty())
        DeclName.print(OS, Policy);
    }

    unsigned NameLen = Blob.size();
    StringRef USR;
    USR = Writer.getUSR(Info.D);
    assert(!USR.empty() && "Recorded decl without USR!");
    Blob += USR;

    if (CGNameGen && ND) {
      llvm::raw_svector_ostream OS(Blob);
      CGNameGen->writeName(ND, OS);
    }

#ifndef NDEBUG
    PresumedLoc PLoc = SM.getPresumedLoc(SM.getFileLoc(Info.D->getLocation()));
    bool IsNew = USRSet.insert(std::make_pair(USR, PLoc)).second;
    if (!IsNew) {
      PresumedLoc OPLoc = USRSet[USR];
      llvm::errs() << "Index: Duplicate USR!\n"
       << OPLoc.getFilename() << ':' << OPLoc.getLine() << ':' << OPLoc.getColumn() << '\n'
       << PLoc.getFilename() << ':' << PLoc.getLine() << ':' << PLoc.getColumn() << '\n';
    }
#endif

    SymbolInfo SymInfo = getSymbolInfo(Info.D);
    assert(SymInfo.Kind != SymbolKind::Unknown);

    Record.clear();
    Record.push_back(REC_DECLINFO);
    Record.push_back((int)SymInfo.Kind);
    Record.push_back((int)SymInfo.Lang);
    Record.push_back((int)SymInfo.TemplateKind);
    Record.push_back(Info.Roles);
    Record.push_back(Info.RelatedRoles);
    Record.push_back(NameLen);
    Record.push_back(USR.size());
    Stream.EmitRecordWithBlob(AbbrevCode, Record, Blob);
  }

  Stream.ExitBlock();

  //===--------------------------------------------------------------------===//
  // DECLOFFSETS_BLOCK_ID
  //===--------------------------------------------------------------------===//

  Stream.EnterSubblock(DECLOFFSETS_BLOCK_ID, 3);

  Abbrev = new BitCodeAbbrev();
  Abbrev->Add(BitCodeAbbrevOp(REC_DECLOFFSETS));
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 32)); // Number of Decls
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Blob)); // Offsets array
  AbbrevCode = Stream.EmitAbbrev(Abbrev);

  Record.clear();
  Record.push_back(REC_DECLOFFSETS);
  Record.push_back(DeclOffsets.size());
  Stream.EmitRecordWithBlob(AbbrevCode, Record, data(DeclOffsets));

  Stream.ExitBlock();

  //===--------------------------------------------------------------------===//
  // DECLOCCURRENCES_BLOCK_ID
  //===--------------------------------------------------------------------===//

  FileID FID = IdxRecord.getFileID();
  auto getLineCol = [&](unsigned Offset) -> std::pair<unsigned, unsigned> {
    unsigned LineNo = SM.getLineNumber(FID, Offset);
    unsigned ColNo = SM.getColumnNumber(FID, Offset);
    return std::make_pair(LineNo, ColNo);
  };

  Stream.EnterSubblock(DECLOCCURRENCES_BLOCK_ID, 3);

  Abbrev = new BitCodeAbbrev();
  Abbrev->Add(BitCodeAbbrevOp(REC_DECLOCCURRENCE));
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 8)); // Decl ID
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, SymbolRoleBitNum)); // Roles
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 12)); // Line
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 8)); // Column
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 4)); // Num related
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Array)); // Related Roles/IDs
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 16)); // Roles or ID
  AbbrevCode = Stream.EmitAbbrev(Abbrev);

  unsigned I = 0;
  for (auto &Occur : IdxRecord.getDeclOccurrences()) {
    const OccurDeclID &OccurIDs = OccurDeclIDs[I++];
    unsigned DeclID = OccurIDs.DeclID;
    unsigned Line, Col;
    std::tie(Line, Col) = getLineCol(Occur.Offset);

    Record.clear();
    Record.push_back(REC_DECLOCCURRENCE);
    Record.push_back(DeclID);
    Record.push_back(Occur.Roles);
    Record.push_back(Line);
    Record.push_back(Col);
    Record.push_back(OccurIDs.RelatedDeclIDs.size());
    unsigned RelI = 0;
    for (unsigned ID : OccurIDs.RelatedDeclIDs) {
      Record.push_back(Occur.Relations[RelI].Roles);
      Record.push_back(ID);
      ++RelI;
    }
    Stream.EmitRecordWithAbbrev(AbbrevCode, Record);
  }
  Stream.ExitBlock();
}
