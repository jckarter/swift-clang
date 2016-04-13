//===--- IndexUnitReader.h - Index unit deserialization -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_INDEX_INDEXUNITREADER_H
#define LLVM_CLANG_INDEX_INDEXUNITREADER_H

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/STLExtras.h"

namespace llvm {
namespace sys {
  class TimeValue;
}
}

namespace clang {
namespace index {

class IndexUnitReader {
public:
  ~IndexUnitReader();

  static std::unique_ptr<IndexUnitReader>
    createWithUnitFilename(StringRef UnitFilename, StringRef StorePath,
                           std::string &Error);
  static std::unique_ptr<IndexUnitReader>
    createWithFilePath(StringRef FilePath, std::string &Error);

  static Optional<llvm::sys::TimeValue>
    getModificationTimeForUnit(StringRef UnitFilename, StringRef StorePath,
                               std::string &Error);

  llvm::sys::TimeValue getModificationTime() const;
  StringRef getWorkingDirectory() const;
  StringRef getOutputFile() const;
  StringRef getTarget() const;

  ArrayRef<StringRef> getDependencyFiles() const;

  /// \c DepIndex is the index in the \c getDependencyFiles array.
  /// Unit dependencies are provided ahead of record ones.
  bool foreachDependency(llvm::function_ref<bool(bool IsUnit,
                                            StringRef UnitOrRecordName,
                                            StringRef Filename,
                                            unsigned DepIndex)> Receiver);

private:
  IndexUnitReader(void *Impl) : Impl(Impl) {}

  void *Impl; // An IndexUnitReaderImpl.
};

} // namespace index
} // namespace clang

#endif
