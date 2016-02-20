//===--- IndexUnitWriter.cpp - Index unit serialization -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "IndexUnitWriter.h"
#include "IndexDataStoreUtils.h"
#include "clang/Basic/FileManager.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::index;
using namespace llvm;

IndexUnitWriter::IndexUnitWriter(StringRef StorePath, StringRef OutputFile,
                                 StringRef TargetTriple) {
  this->UnitsPath = StorePath;
  store::makeUnitSubDir(this->UnitsPath);
  this->OutputFile = OutputFile;
  this->TargetTriple = TargetTriple;
}

IndexUnitWriter::~IndexUnitWriter() {}

unsigned IndexUnitWriter::addDependency(const FileEntry *File) {
  assert(File);
  auto Pair = IndexByFile.insert(std::make_pair(File, Files.size()));
  bool WasInserted = Pair.second;
  if (WasInserted) {
    Files.emplace_back(File);
  }
  return Pair.first->second;
}

void IndexUnitWriter::addRecordFile(StringRef RecordFile, const FileEntry *File) {
  assert(File);
  Records.emplace_back(RecordFile, addDependency(File));
}

bool IndexUnitWriter::write(std::string &Error) {
  using namespace llvm::sys;

  llvm::hash_code PathHashVal = llvm::hash_value(OutputFile);
  std::string PathHashString = llvm::APInt(64, PathHashVal).toString(36, /*Signed=*/false);

  SmallString<256> UnitPath = StringRef(UnitsPath);
  UnitPath += '/';
  UnitPath += path::filename(OutputFile);
  UnitPath += '-';
  UnitPath += PathHashString;

  SmallString<128> TempPath;
  TempPath = path::parent_path(UnitsPath);
  TempPath += '/';
  TempPath += path::filename(UnitPath);
  TempPath += "-%%%%%%%%";
  int TempFD;
  if (llvm::sys::fs::createUniqueFile(TempPath.str(), TempFD, TempPath)) {
    Error = "failed creating temporary file: ";
    Error += TempPath.str();
    return true;
  }

  raw_fd_ostream OS(TempFD, /*shouldClose=*/true);

  // Write the working directory.

  SmallString<128> CWDPath;
  std::error_code EC = fs::current_path(CWDPath);
  if (EC) {
    Error = EC.message();
    return true;
  }
  OS << CWDPath.str() << '\n';

  // Write the output file.

  OS << OutputFile << '\n';

  // Write the target triple.

  OS << TargetTriple << '\n';

  // Write the dependencies.

  for (auto *FE : Files) {
    OS << FE->getName() << '\n';
  }
  OS << "===\n";

  // Write pairs of record file and index in the dependencies list.

  for (const auto &Pair : Records) {
    OS << Pair.first << '\n';
    OS << Pair.second << '\n';
  }

  OS.close();

  EC = fs::rename(/*from=*/TempPath.c_str(), /*to=*/UnitPath.c_str());
  if (EC) {
    Error = EC.message();
    return true;
  }

  return false;
}
