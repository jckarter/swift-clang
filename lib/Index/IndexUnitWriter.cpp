//===--- IndexUnitWriter.cpp - Index unit serialization -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Index/IndexUnitWriter.h"
#include "IndexDataStoreUtils.h"
#include "clang/Basic/FileManager.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/Bitcode/BitstreamWriter.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::index;
using namespace clang::index::store;
using namespace llvm;

IndexUnitWriter::IndexUnitWriter(StringRef StorePath, StringRef OutputFile,
                                 StringRef TargetTriple) {
  this->UnitsPath = StorePath;
  store::makeUnitSubDir(this->UnitsPath);
  this->OutputFile = OutputFile;
  this->TargetTriple = TargetTriple;
}

IndexUnitWriter::~IndexUnitWriter() {}

int IndexUnitWriter::addFileDependency(const FileEntry *File) {
  assert(File);
  auto Pair = IndexByFile.insert(std::make_pair(File, Files.size()));
  bool WasInserted = Pair.second;
  if (WasInserted) {
    Files.emplace_back(File);
  }
  return Pair.first->second;
}

void IndexUnitWriter::addRecordFile(StringRef RecordFile, const FileEntry *File) {
  int Dep = File ? addFileDependency(File) : -1;
  Records.emplace_back(RecordFile, Dep);
}

void IndexUnitWriter::addASTFileDependency(const FileEntry *File) {
  assert(File);
  if (!SeenASTFiles.insert(File).second)
    return;

  SmallString<64> UnitName;
  getUnitNameForOutputFile(File->getName(), UnitName);
  addUnitDependency(UnitName.str(), File);
}

void IndexUnitWriter::addUnitDependency(StringRef UnitFile,
                                        const FileEntry *File) {
  int Dep = File ? addFileDependency(File) : -1;
  ASTFileUnits.emplace_back(UnitFile, Dep);
}

void IndexUnitWriter::getUnitNameForOutputFile(StringRef FilePath,
                                               SmallVectorImpl<char> &Str) {
  StringRef Fname = sys::path::filename(FilePath);
  Str.append(Fname.begin(), Fname.end());
  Str.push_back('-');
  llvm::hash_code PathHashVal = llvm::hash_value(FilePath);
  llvm::APInt(64, PathHashVal).toString(Str, 36, /*Signed=*/false);
}

static void writeBlockInfo(BitstreamWriter &Stream) {
  RecordData Record;

  Stream.EnterBlockInfoBlock(3);
#define BLOCK(X) emitBlockID(X ## _ID, #X, Stream, Record)
#define RECORD(X) emitRecordID(X, #X, Stream, Record)

  BLOCK(UNIT_VERSION_BLOCK);
  RECORD(UNIT_VERSION);

  BLOCK(UNIT_INFO_BLOCK);
  RECORD(UNIT_INFO);

  BLOCK(UNIT_DEPENDENCIES_BLOCK);
  RECORD(UNIT_DEPENDENCY);

  BLOCK(UNIT_PATHS_BLOCK);
  RECORD(UNIT_PATHS);

#undef RECORD
#undef BLOCK
  Stream.ExitBlock();
}

static void writeVersionInfo(BitstreamWriter &Stream) {
  using namespace llvm::sys;

  Stream.EnterSubblock(UNIT_VERSION_BLOCK_ID, 3);

  BitCodeAbbrev *Abbrev = new BitCodeAbbrev();
  Abbrev->Add(BitCodeAbbrevOp(UNIT_VERSION));
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 6)); // Store format version
  unsigned AbbrevCode = Stream.EmitAbbrev(Abbrev);

  RecordData Record;
  Record.push_back(UNIT_VERSION);
  Record.push_back(STORE_FORMAT_VERSION);
  Stream.EmitRecordWithAbbrev(AbbrevCode, Record);

  Stream.ExitBlock();
}

bool IndexUnitWriter::write(std::string &Error) {
  using namespace llvm::sys;

  SmallString<512> Buffer;
  BitstreamWriter Stream(Buffer);
  Stream.Emit('I', 8);
  Stream.Emit('D', 8);
  Stream.Emit('X', 8);
  Stream.Emit('U', 8);

  SmallString<512> PathsBuf;
  writeBlockInfo(Stream);
  writeVersionInfo(Stream);
  if (writeUnitInfo(Stream, PathsBuf, Error))
    return true;
  writeDependencies(Stream, PathsBuf);
  writePaths(Stream, PathsBuf);

  SmallString<256> UnitPath = StringRef(UnitsPath);
  UnitPath += '/';
  getUnitNameForOutputFile(OutputFile, UnitPath);

  SmallString<128> TempPath;
  TempPath = path::parent_path(UnitsPath);
  TempPath += '/';
  TempPath += path::filename(UnitPath);
  TempPath += "-%%%%%%%%";
  int TempFD;
  if (llvm::sys::fs::createUniqueFile(TempPath.str(), TempFD, TempPath)) {
    llvm::raw_string_ostream Err(Error);
    Err << "failed to create temporary file: " << TempPath;
    return true;
  }

  raw_fd_ostream OS(TempFD, /*shouldClose=*/true);
  OS.write(Buffer.data(), Buffer.size());
  OS.close();

  std::error_code EC = fs::rename(/*from=*/TempPath.c_str(), /*to=*/UnitPath.c_str());
  if (EC) {
    llvm::raw_string_ostream Err(Error);
    Err << "failed to rename '" << TempPath << "' to '" << UnitPath << "': " << EC.message();
    return true;
  }

  return false;
}

static size_t getPathOffset(StringRef Path, SmallString<512> &PathsBuf) {
  size_t offset = PathsBuf.size();
  PathsBuf += Path;
  return offset;
}

bool IndexUnitWriter::writeUnitInfo(llvm::BitstreamWriter &Stream, SmallString<512> &PathsBuf,
                                    std::string &Error) {
  // Write the working directory.

  SmallString<128> CWDPath;
  std::error_code EC = sys::fs::current_path(CWDPath);
  if (EC) {
    llvm::raw_string_ostream Err(Error);
    Err << "failed to determine current working directory: " << EC.message();
    return true;
  }

  Stream.EnterSubblock(UNIT_INFO_BLOCK_ID, 3);

  BitCodeAbbrev *Abbrev = new BitCodeAbbrev();
  Abbrev->Add(BitCodeAbbrevOp(UNIT_INFO));
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 8)); // WorkDir offset
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 7)); // WorkDir size
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 8)); // OutputFile offset
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 7)); // OutputFile size
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Blob)); // target triple
  unsigned AbbrevCode = Stream.EmitAbbrev(Abbrev);

  RecordData Record;
  Record.push_back(UNIT_INFO);
  Record.push_back(getPathOffset(CWDPath, PathsBuf));
  Record.push_back(CWDPath.size());
  Record.push_back(getPathOffset(OutputFile, PathsBuf));
  Record.push_back(OutputFile.size());
  Stream.EmitRecordWithBlob(AbbrevCode, Record, TargetTriple);

  Stream.ExitBlock();
  return false;
}

void IndexUnitWriter::writeDependencies(llvm::BitstreamWriter &Stream, SmallString<512> &PathsBuf) {
  struct PathData {
    size_t Offset;
    size_t Size;
    bool UsedForRecordOrUnit;
  };
  SmallVector<PathData, 32> PathDataEntries;
  PathDataEntries.reserve(Files.size());
  for (auto *FE : Files) {
    StringRef Path = FE->getName();
    PathData Entry{getPathOffset(Path, PathsBuf), Path.size(), false};
    PathDataEntries.push_back(Entry);
  }

  Stream.EnterSubblock(UNIT_DEPENDENCIES_BLOCK_ID, 3);

  BitCodeAbbrev *Abbrev = new BitCodeAbbrev();
  Abbrev->Add(BitCodeAbbrevOp(UNIT_DEPENDENCY));
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, UnitDependencyKindBitNum)); // Kind
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 8)); // Path offset
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 7)); // Path size
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Blob)); // Name
  unsigned AbbrevCode = Stream.EmitAbbrev(Abbrev);

  RecordData Record;
  for (auto &pair : ASTFileUnits) {
    Record.clear();
    Record.push_back(UNIT_DEPENDENCY);
    Record.push_back(UNIT_DEPEND_KIND_UNIT);
    if (pair.second != -1) {
      PathData &pathEntry = PathDataEntries[pair.second];
      Record.push_back(pathEntry.Offset);
      Record.push_back(pathEntry.Size);
      pathEntry.UsedForRecordOrUnit = true;
    } else {
      Record.push_back(0);
      Record.push_back(0);
    }
    Stream.EmitRecordWithBlob(AbbrevCode, Record, pair.first);
  }
  for (auto &pair : Records) {
    Record.clear();
    Record.push_back(UNIT_DEPENDENCY);
    Record.push_back(UNIT_DEPEND_KIND_RECORD);
    PathData &pathEntry = PathDataEntries[pair.second];
    Record.push_back(pathEntry.Offset);
    Record.push_back(pathEntry.Size);
    pathEntry.UsedForRecordOrUnit = true;
    Stream.EmitRecordWithBlob(AbbrevCode, Record, pair.first);
  }
  for (auto &pathEntry : PathDataEntries) {
    if (pathEntry.UsedForRecordOrUnit)
      continue;
    Record.clear();
    Record.push_back(UNIT_DEPENDENCY);
    Record.push_back(UNIT_DEPEND_KIND_FILE);
    Record.push_back(pathEntry.Offset);
    Record.push_back(pathEntry.Size);
    Stream.EmitRecordWithBlob(AbbrevCode, Record, StringRef());
  }
  Stream.ExitBlock();
}

void IndexUnitWriter::writePaths(llvm::BitstreamWriter &Stream, SmallString<512> &PathsBuf) {
  Stream.EnterSubblock(UNIT_PATHS_BLOCK_ID, 3);

  BitCodeAbbrev *Abbrev = new BitCodeAbbrev();
  Abbrev->Add(BitCodeAbbrevOp(UNIT_PATHS));
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Blob)); // Paths buffer
  unsigned AbbrevCode = Stream.EmitAbbrev(Abbrev);

  RecordData Record;
  Record.push_back(UNIT_PATHS);
  Stream.EmitRecordWithBlob(AbbrevCode, Record, PathsBuf);

  Stream.ExitBlock();
}

bool IndexUnitWriter::initIndexDirectory(StringRef StorePath,
                                         std::string &Error) {
  using namespace llvm::sys;
  SmallString<128> SubPath = StorePath;
  store::makeRecordSubDir(SubPath);
  std::error_code EC = fs::create_directories(SubPath);
  if (EC) {
    llvm::raw_string_ostream Err(Error);
    Err << "failed to create directory '" << SubPath << "': " << EC.message();
    return true;
  }

  SubPath = StorePath;
  store::makeUnitSubDir(SubPath);
  EC = fs::create_directory(SubPath);
  if (EC) {
    llvm::raw_string_ostream Err(Error);
    Err << "failed to create directory '" << SubPath << "': " << EC.message();
    return true;
  }

  return false;
}
