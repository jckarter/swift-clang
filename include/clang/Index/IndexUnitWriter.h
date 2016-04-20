//===--- IndexUnitWriter.h - Index unit serialization ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_INDEX_INDEXUNITWRITER_H
#define LLVM_CLANG_INDEX_INDEXUNITWRITER_H

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallString.h"
#include <string>
#include <vector>

namespace clang {
  class Decl;
  class FileEntry;

namespace index {

class IndexUnitWriter {
  SmallString<64> UnitsPath;
  std::string OutputFile;
  std::string TargetTriple;
  std::vector<const FileEntry *> Files;
  llvm::DenseMap<const FileEntry *, int> IndexByFile;
  std::vector<std::pair<std::string, int>> Records;
  std::vector<std::pair<std::string, int>> ASTFileUnits;

public:
  IndexUnitWriter(StringRef StorePath, StringRef OutputFile,
                  StringRef TargetTriple);
  ~IndexUnitWriter();

  int addFileDependency(const FileEntry *File);
  void addRecordFile(StringRef RecordFile, const FileEntry *File);
  void addASTFileDependency(const FileEntry *File);
  void addUnitDependency(StringRef UnitFile, const FileEntry *File);

  bool write(std::string &Error);

  static void getUnitNameForOutputFile(StringRef FilePath,
                                       SmallVectorImpl<char> &Str);
  static bool initIndexDirectory(StringRef StorePath, std::string &Error);
};

} // end namespace index
} // end namespace clang

#endif
