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
#include "BitstreamVisitor.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Bitcode/BitstreamReader.h"
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

typedef llvm::function_ref<bool(IndexUnitReader::DependencyKind Kind,
                                StringRef UnitOrRecordName,
                                StringRef FilePath)> DependencyReceiver;

class IndexUnitReaderImpl {
  sys::TimeValue ModTime;
  std::unique_ptr<MemoryBuffer> MemBuf;
  llvm::BitstreamReader BitReader;

public:
  llvm::BitstreamCursor DependCursor;
  StringRef WorkingDir;
  StringRef OutputFile;
  StringRef SysrootPath;
  StringRef Target;
  StringRef PathsBuffer;

  bool init(std::unique_ptr<MemoryBuffer> Buf, sys::TimeValue ModTime,
            std::string &Error);

  llvm::sys::TimeValue getModificationTime() const { return ModTime; }
  StringRef getWorkingDirectory() const { return WorkingDir; }
  StringRef getOutputFile() const { return OutputFile; }
  StringRef getSysrootPath() const { return SysrootPath; }
  StringRef getTarget() const { return Target; }

  /// Unit dependencies are provided ahead of record ones, record ones
  /// ahead of the file ones.
  bool foreachDependency(DependencyReceiver Receiver);

  StringRef getPathFromBuffer(size_t Offset, size_t Size) {
    return PathsBuffer.substr(Offset, Size);
  }
};

class IndexUnitBitstreamVisitor : public BitstreamVisitor<IndexUnitBitstreamVisitor> {
  IndexUnitReaderImpl &Reader;
  size_t WorkDirOffset;
  size_t WorkDirSize;
  size_t OutputFileOffset;
  size_t OutputFileSize;
  size_t SysrootOffset;
  size_t SysrootSize;

public:
  IndexUnitBitstreamVisitor(llvm::BitstreamCursor &Stream,
                            IndexUnitReaderImpl &Reader)
    : BitstreamVisitor(Stream), Reader(Reader) {}

  StreamVisit visitBlock(unsigned ID) {
    switch ((UnitBitBlock)ID) {
    case UNIT_VERSION_BLOCK_ID:
    case UNIT_INFO_BLOCK_ID:
    case UNIT_PATHS_BLOCK_ID:
      return StreamVisit::Continue;

    case UNIT_DEPENDENCIES_BLOCK_ID:
      Reader.DependCursor = Stream;
      if (Reader.DependCursor.EnterSubBlock(ID)) {
        *Error = "malformed block record";
        return StreamVisit::Abort;
      }
      readBlockAbbrevs(Reader.DependCursor);
      return StreamVisit::Skip;
    }

    // Some newly introduced block in a minor version update that we cannot
    // handle.
    return StreamVisit::Skip;
  }

  StreamVisit visitRecord(unsigned BlockID, unsigned RecID,
                          RecordDataImpl &Record, StringRef Blob) {
    switch (BlockID) {
    case UNIT_VERSION_BLOCK_ID: {
      unsigned StoreFormatVersion = Record[0];
      if (StoreFormatVersion != STORE_FORMAT_VERSION) {
        llvm::raw_string_ostream OS(*Error);
        OS << "Store format version mismatch: " << StoreFormatVersion;
        OS << " , expected: " << STORE_FORMAT_VERSION;
        return StreamVisit::Abort;
      }
      break;
    }

    case UNIT_INFO_BLOCK_ID: {
      assert(RecID == UNIT_INFO);
      unsigned I = 0;
      // Save these to lookup them up after we get the paths buffer.
      WorkDirOffset = Record[I++];
      WorkDirSize = Record[I++];
      OutputFileOffset = Record[I++];
      OutputFileSize = Record[I++];
      SysrootOffset = Record[I++];
      SysrootSize = Record[I++];
      Reader.Target = Blob;
      break;
    }

    case UNIT_PATHS_BLOCK_ID:
      assert(RecID == UNIT_PATHS);
      Reader.PathsBuffer = Blob;
      Reader.WorkingDir = Reader.getPathFromBuffer(WorkDirOffset, WorkDirSize);
      Reader.OutputFile = Reader.getPathFromBuffer(OutputFileOffset, OutputFileSize);
      Reader.SysrootPath = Reader.getPathFromBuffer(SysrootOffset, SysrootSize);
      break;

    case UNIT_DEPENDENCIES_BLOCK_ID:
      llvm_unreachable("shouldn't visit this block'");
    }
    return StreamVisit::Continue;
  }
};

} // anonymous namespace

bool IndexUnitReaderImpl::init(std::unique_ptr<MemoryBuffer> Buf,
                               sys::TimeValue ModTime, std::string &Error) {
  this->ModTime = ModTime;
  this->MemBuf = std::move(Buf);
  this->BitReader.init((const unsigned char *)MemBuf->getBufferStart(),
                       (const unsigned char *)MemBuf->getBufferEnd());

  llvm::BitstreamCursor Stream(BitReader);

  // Sniff for the signature.
  if (Stream.Read(8) != 'I' ||
      Stream.Read(8) != 'D' ||
      Stream.Read(8) != 'X' ||
      Stream.Read(8) != 'U') {
    Error = "not a serialized index unit file";
    return true;
  }

  IndexUnitBitstreamVisitor BitVisitor(Stream, *this);
  if (!BitVisitor.visit(Error))
    return true;

  return false;
}

/// Unit dependencies are provided ahead of record ones, record ones
/// ahead of the file ones.
bool IndexUnitReaderImpl::foreachDependency(DependencyReceiver Receiver) {
  class DependBitVisitor : public BitstreamVisitor<DependBitVisitor> {
    IndexUnitReaderImpl &Reader;
    DependencyReceiver Receiver;

  public:
    DependBitVisitor(llvm::BitstreamCursor &Stream,
                     IndexUnitReaderImpl &Reader,
                     DependencyReceiver Receiver)
      : BitstreamVisitor(Stream),
        Reader(Reader),
        Receiver(std::move(Receiver)) {}

    StreamVisit visitRecord(unsigned BlockID, unsigned RecID,
                            RecordDataImpl &Record, StringRef Blob) {
      assert(RecID == UNIT_DEPENDENCY);
      unsigned I = 0;
      UnitDependencyKind DK = (UnitDependencyKind)Record[I++];
      UnitFilePathPrefixKind prefixKind = (UnitFilePathPrefixKind)Record[I++];
      size_t dirPathOffset = Record[I++];
      size_t dirPathSize = Record[I++];
      size_t fnameOffset = Record[I++];
      size_t fnameSize = Record[I++];
      StringRef name = Blob;

      IndexUnitReader::DependencyKind DepKind;
      switch (DK) {
      case UNIT_DEPEND_KIND_UNIT:
        DepKind = IndexUnitReader::DependencyKind::Unit; break;
      case UNIT_DEPEND_KIND_RECORD:
        DepKind = IndexUnitReader::DependencyKind::Record; break;
      case UNIT_DEPEND_KIND_FILE:
        DepKind = IndexUnitReader::DependencyKind::File; break;
      }

      SmallString<512> pathBuf;
      switch (prefixKind) {
      case UNIT_PATH_PREFIX_NONE:
        break;
      case UNIT_PATH_PREFIX_WORKDIR:
        pathBuf = Reader.getWorkingDirectory();
        break;
      case UNIT_PATH_PREFIX_SYSROOT:
        pathBuf = Reader.getSysrootPath();
        break;
      }
      sys::path::append(pathBuf,
                        Reader.getPathFromBuffer(dirPathOffset, dirPathSize),
                        Reader.getPathFromBuffer(fnameOffset, fnameSize));
      if (!Receiver(DepKind, name, pathBuf.str()))
        return StreamVisit::Abort;
      return StreamVisit::Continue;
    }
  };

  SavedStreamPosition SavedPosition(DependCursor);
  DependBitVisitor Visitor(DependCursor, *this, Receiver);
  std::string Error;
  return Visitor.visit(Error);
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
    return None;
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

StringRef IndexUnitReader::getSysrootPath() const {
  return IMPL->getSysrootPath();
}

StringRef IndexUnitReader::getTarget() const {
  return IMPL->getTarget();
}

/// \c Index is the index in the \c getDependencies array.
/// Unit dependencies are provided ahead of record ones.
bool IndexUnitReader::foreachDependency(llvm::function_ref<bool(DependencyKind Kind,
                                            StringRef UnitOrRecordName,
                                            StringRef FilePath)> Receiver) {
  return IMPL->foreachDependency(std::move(Receiver));
}
