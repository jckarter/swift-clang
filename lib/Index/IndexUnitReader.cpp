//===--- IndexUnitReader.cpp - Index unit deserialization -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Index/IndexUnitReader.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/TimeValue.h"

#include <unistd.h>

using namespace clang;
using namespace clang::index;
using namespace llvm;

namespace {

class IndexUnitReaderImpl {
  std::unique_ptr<MemoryBuffer> MemBuf;
  sys::TimeValue ModTime;
  StringRef WorkingDir;
  StringRef OutputFile;
  StringRef Target;
  SmallVector<StringRef, 16> Dependencies;
  SmallVector<std::pair<StringRef, unsigned>, 16> Records;

public:
  bool init(std::unique_ptr<MemoryBuffer> Buf, sys::TimeValue ModTime,
            std::string &Error);

  llvm::sys::TimeValue getModificationTime() const;
  StringRef getWorkingDirectory() const;
  StringRef getOutputFile() const;
  StringRef getTarget() const;

  ArrayRef<StringRef> getDependencies() const;

  /// \c Index is the index in the \c getDependencies array.
  bool foreachRecord(llvm::function_ref<bool(StringRef RecordFile,
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

  while (!Buffer.empty()) {
    StringRef RecordFile, IndexStr;
    std::tie(RecordFile, Buffer) = Buffer.split('\n');
    std::tie(IndexStr, Buffer) = Buffer.split('\n');
    unsigned Index;
    bool Err = IndexStr.getAsInteger(10, Index);
    if (Err) {
      Error = "Error getting index for record: ";
      Error += RecordFile;
      return true;
    }
    if (Index >= Dependencies.size()) {
      Error = "Out of range index for record: ";
      Error += RecordFile;
      return true;
    }
    Records.push_back(std::make_pair(RecordFile, Index));
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

ArrayRef<StringRef> IndexUnitReaderImpl::getDependencies() const {
  return Dependencies;
}

/// \c Index is the index in the \c getDependencies array.
bool IndexUnitReaderImpl::foreachRecord(llvm::function_ref<bool(StringRef RecordFile,
                                                                StringRef Filename,
                                                                unsigned DepIndex)> Receiver) {
  for (auto &Pair : Records) {
    unsigned Index = Pair.second;
    bool Continue = Receiver(Pair.first,  Dependencies[Index], Index);
    if (!Continue)
      return false;
  }
  return true;
}

//===----------------------------------------------------------------------===//
// IndexUnitReader
//===----------------------------------------------------------------------===//

std::unique_ptr<IndexUnitReader>
IndexUnitReader::create(StringRef Filename, std::string &Error) {
  int FD;
  std::error_code EC = sys::fs::openFileForRead(Filename, FD);
  if (EC) {
    Error = "Failed opening '";
    Error += Filename;
    Error += "': ";
    Error += EC.message();
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

  auto ErrOrBuf = MemoryBuffer::getOpenFile(FD, Filename, /*FileSize=*/-1,
                                            /*RequiresNullTerminator=*/false);
  if (!ErrOrBuf) {
    Error = "Failed opening '";
    Error += Filename;
    Error += "': ";
    Error += ErrOrBuf.getError().message();
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

ArrayRef<StringRef> IndexUnitReader::getDependencies() const {
  return IMPL->getDependencies();
}

/// \c Index is the index in the \c getDependencies array.
bool IndexUnitReader::foreachRecord(llvm::function_ref<bool(StringRef RecordFile,
                                                            StringRef Filename,
                                                            unsigned DepIndex)> Receiver) {
  return IMPL->foreachRecord(std::move(Receiver));
}
