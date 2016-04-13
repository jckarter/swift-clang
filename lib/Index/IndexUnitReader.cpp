//===--- IndexUnitReader.cpp - Index unit deserialization -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Index/IndexUnitReader.h"
#include "IndexDataStoreUtils.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TimeValue.h"
#include "llvm/Support/raw_ostream.h"

#include <unistd.h>

using namespace clang;
using namespace clang::index;
using namespace clang::index::store;
using namespace llvm;

namespace {

class IndexUnitReaderImpl {
  std::unique_ptr<MemoryBuffer> MemBuf;
  sys::TimeValue ModTime;
  StringRef WorkingDir;
  StringRef OutputFile;
  StringRef Target;
  SmallVector<StringRef, 16> Dependencies;
  SmallVector<std::pair<StringRef, unsigned>, 6> ASTFileUnits;
  SmallVector<std::pair<StringRef, unsigned>, 16> Records;

public:
  bool init(std::unique_ptr<MemoryBuffer> Buf, sys::TimeValue ModTime,
            std::string &Error);

  llvm::sys::TimeValue getModificationTime() const;
  StringRef getWorkingDirectory() const;
  StringRef getOutputFile() const;
  StringRef getTarget() const;

  ArrayRef<StringRef> getDependencyFiles() const;

  /// \c Index is the index in the \c getDependencies array.
  /// Unit dependencies are provided ahead of record ones.
  bool foreachDependency(llvm::function_ref<bool(bool IsUnit,
                                            StringRef UnitOrRecordName,
                                            StringRef Filename,
                                            unsigned DepIndex)> Receiver);
};

} // anonymous namespace

bool IndexUnitReaderImpl::init(std::unique_ptr<MemoryBuffer> Buf,
                               sys::TimeValue ModTime, std::string &Error) {
  this->ModTime = ModTime;
  StringRef Buffer = Buf->getBuffer();
  std::tie(WorkingDir, Buffer) = Buffer.split('\n');
  std::tie(OutputFile, Buffer) = Buffer.split('\n');
  std::tie(Target, Buffer) = Buffer.split('\n');

  while (!Buffer.empty()) {
    StringRef Dep;
    std::tie(Dep, Buffer) = Buffer.split('\n');
    if (Dep == "===")
      break;
    Dependencies.push_back(Dep);
  }

  bool InUnitSection = true;
  while (!Buffer.empty()) {
    StringRef UnitOrRecordName, IndexStr;
    std::tie(UnitOrRecordName, Buffer) = Buffer.split('\n');
    if (UnitOrRecordName == "===") {
      InUnitSection = false;
      continue;
    }
    std::tie(IndexStr, Buffer) = Buffer.split('\n');
    unsigned Index;
    bool Err = IndexStr.getAsInteger(10, Index);
    if (Err) {
      Error = "Error getting index for dependency: ";
      Error += UnitOrRecordName;
      return true;
    }
    if (Index >= Dependencies.size()) {
      Error = "Out of range index for dependency: ";
      Error += UnitOrRecordName;
      return true;
    }
    if (InUnitSection)
      ASTFileUnits.push_back(std::make_pair(UnitOrRecordName, Index));
    else
      Records.push_back(std::make_pair(UnitOrRecordName, Index));
  }

  MemBuf = std::move(Buf);

  return false;
}

llvm::sys::TimeValue IndexUnitReaderImpl::getModificationTime() const {
  return ModTime;
}

StringRef IndexUnitReaderImpl::getWorkingDirectory() const {
  return WorkingDir;
}

StringRef IndexUnitReaderImpl::getOutputFile() const {
  return OutputFile;
}

StringRef IndexUnitReaderImpl::getTarget() const {
  return Target;
}

ArrayRef<StringRef> IndexUnitReaderImpl::getDependencyFiles() const {
  return Dependencies;
}

/// \c Index is the index in the \c getDependencies array.
/// Unit dependencies are provided ahead of record ones.
bool IndexUnitReaderImpl::foreachDependency(llvm::function_ref<bool(bool IsUnit,
                                            StringRef UnitOrRecordName,
                                            StringRef Filename,
                                            unsigned DepIndex)> Receiver) {
  for (auto &Pair : ASTFileUnits) {
    unsigned Index = Pair.second;
    bool Continue = Receiver(/*IsUnit=*/true, Pair.first,  Dependencies[Index],
                             Index);
    if (!Continue)
      return false;
  }
  for (auto &Pair : Records) {
    unsigned Index = Pair.second;
    bool Continue = Receiver(/*IsUnit=*/false, Pair.first,  Dependencies[Index],
                             Index);
    if (!Continue)
      return false;
  }
  return true;
}

//===----------------------------------------------------------------------===//
// IndexUnitReader
//===----------------------------------------------------------------------===//

std::unique_ptr<IndexUnitReader>
IndexUnitReader::createWithUnitFilename(StringRef UnitFilename,
                                        StringRef StorePath,
                                        std::string &Error) {
  SmallString<128> PathBuf = StorePath;
  makeUnitSubDir(PathBuf);
  sys::path::append(PathBuf, UnitFilename);
  return createWithFilePath(PathBuf.str(), Error);
}

std::unique_ptr<IndexUnitReader>
IndexUnitReader::createWithFilePath(StringRef FilePath, std::string &Error) {
  int FD;
  std::error_code EC = sys::fs::openFileForRead(FilePath, FD);
  if (EC) {
    raw_string_ostream(Error) << "Failed opening '" << FilePath << "': "
      << EC.message();
    return nullptr;
  }

  assert(FD != -1);
  struct AutoFDClose {
    int FD;
    AutoFDClose(int FD) : FD(FD) {}
    ~AutoFDClose() {
      ::close(FD);
    }
  } AutoFDClose(FD);

  sys::fs::file_status FileStat;
  EC = sys::fs::status(FD, FileStat);
  if (EC) {
    Error = EC.message();
    return nullptr;
  }

  auto ErrOrBuf = MemoryBuffer::getOpenFile(FD, FilePath, /*FileSize=*/-1,
                                            /*RequiresNullTerminator=*/false);
  if (!ErrOrBuf) {
    raw_string_ostream(Error) << "Failed opening '" << FilePath << "': "
      << ErrOrBuf.getError().message();
    return nullptr;
  }

  std::unique_ptr<IndexUnitReaderImpl> Impl(new IndexUnitReaderImpl());
  bool Err = Impl->init(std::move(*ErrOrBuf), FileStat.getLastModificationTime(),
                        Error);
  if (Err)
    return nullptr;

  std::unique_ptr<IndexUnitReader> Reader;
  Reader.reset(new IndexUnitReader(Impl.release()));
  return Reader;
}

Optional<sys::TimeValue>
IndexUnitReader::getModificationTimeForUnit(StringRef UnitFilename,
                                            StringRef StorePath,
                                            std::string &Error) {
  SmallString<128> PathBuf = StorePath;
  makeUnitSubDir(PathBuf);
  sys::path::append(PathBuf, UnitFilename);

  sys::fs::file_status FileStat;
  std::error_code EC = sys::fs::status(PathBuf.str(), FileStat);
  if (EC) {
    Error = EC.message();
    return sys::TimeValue();
  }
  return FileStat.getLastModificationTime();
}

#define IMPL static_cast<IndexUnitReaderImpl*>(Impl)

IndexUnitReader::~IndexUnitReader() {
  delete IMPL;
}

llvm::sys::TimeValue IndexUnitReader::getModificationTime() const {
  return IMPL->getModificationTime();
}

StringRef IndexUnitReader::getWorkingDirectory() const {
  return IMPL->getWorkingDirectory();
}

StringRef IndexUnitReader::getOutputFile() const {
  return IMPL->getOutputFile();
}

StringRef IndexUnitReader::getTarget() const {
  return IMPL->getTarget();
}

ArrayRef<StringRef> IndexUnitReader::getDependencyFiles() const {
  return IMPL->getDependencyFiles();
}

/// \c Index is the index in the \c getDependencies array.
/// Unit dependencies are provided ahead of record ones.
bool IndexUnitReader::foreachDependency(llvm::function_ref<bool(bool IsUnit,
                                            StringRef UnitOrRecordName,
                                            StringRef Filename,
                                            unsigned DepIndex)> Receiver) {
  return IMPL->foreachDependency(std::move(Receiver));
}
