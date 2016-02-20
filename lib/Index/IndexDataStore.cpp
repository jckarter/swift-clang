//===--- IndexDataStore.cpp - Index data store info -----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Index/IndexDataStore.h"
#include "IndexDataStoreUtils.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::index;
using namespace clang::index::store;
using namespace llvm;

static void makeSubDir(StringRef subdir, SmallVectorImpl<char> &StorePathBuf) {
  SmallString<10> VersionPath;
  raw_svector_ostream(VersionPath) << 'v' << STORE_FORMAT_VERSION;

  sys::path::append(StorePathBuf, VersionPath);
  sys::path::append(StorePathBuf, subdir);
}

void store::makeUnitSubDir(SmallVectorImpl<char> &StorePathBuf) {
  return makeSubDir("units", StorePathBuf);
}

void store::makeRecordSubDir(SmallVectorImpl<char> &StorePathBuf) {
  return makeSubDir("records", StorePathBuf);
}

//===----------------------------------------------------------------------===//
// IndexDataStore
//===----------------------------------------------------------------------===//

namespace {

class IndexDataStoreImpl {
  SmallString<128> UnitPath;

public:
  IndexDataStoreImpl(StringRef indexStorePath) {
    UnitPath = indexStorePath;
    makeUnitSubDir(UnitPath);
  }

  std::vector<std::string> getAllUnitPaths() const;
};

} // anonymous namespace

std::vector<std::string> IndexDataStoreImpl::getAllUnitPaths() const {
  std::vector<std::string> paths;

  std::error_code EC;
  for (auto It = sys::fs::directory_iterator(UnitPath, EC), End = sys::fs::directory_iterator();
       !EC && It != End; It.increment(EC)) {
    paths.push_back(It->path());
  }

  return paths;
}

std::unique_ptr<IndexDataStore>
IndexDataStore::create(StringRef IndexStorePath, std::string &Error) {
  if (!sys::fs::exists(IndexStorePath)) {
    raw_string_ostream OS(Error);
    OS << "index store path does not exist: " << IndexStorePath;
    return nullptr;
  }

  return std::unique_ptr<IndexDataStore>(
    new IndexDataStore(new IndexDataStoreImpl(IndexStorePath)));
}

#define IMPL static_cast<IndexDataStoreImpl*>(Impl)

IndexDataStore::~IndexDataStore() {
  delete IMPL;
}

std::vector<std::string> IndexDataStore::getAllUnitPaths() const {
  return IMPL->getAllUnitPaths();
}
