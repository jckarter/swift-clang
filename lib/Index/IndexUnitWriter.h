//===--- IndexUnitWriter.h - Index unit serialization ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_INDEX_INDEXUNITWRITER_H
#define LLVM_CLANG_LIB_INDEX_INDEXUNITWRITER_H

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallString.h"
#include <string>
#include <vector>

namespace clang {
  class Decl;
  class FileEntry;

namespace index {
  class FileIndexRecord;

class IndexUnitWriter {
  SmallString<64> UnitsPath;
  std::string OutputFile;
  std::string TargetTriple;
  std::vector<const FileEntry *> Files;
  llvm::DenseMap<const FileEntry *, unsigned> IndexByFile;
  std::vector<std::pair<std::string, unsigned>> Records;
  std::vector<std::pair<std::string, unsigned>> ASTFileUnits;

public:
  IndexUnitWriter(StringRef StorePath, StringRef OutputFile,
                  StringRef TargetTriple);
  ~IndexUnitWriter();

  unsigned addFileDependency(const FileEntry *File);
  void addRecordFile(StringRef RecordFile, const FileEntry *File);
  void addASTFileDependency(const FileEntry *File);

  bool write(std::string &Error);

  static void getUnitNameForOutputFile(StringRef FilePath,
                                       SmallVectorImpl<char> &Str);
};

} // end namespace index
} // end namespace clang

#endif
