//===--- IndexDataStoreUtils.h - Functions/constants for the data store ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_INDEX_INDEXDATASTOREUTILS_H
#define LLVM_CLANG_LIB_INDEX_INDEXDATASTOREUTILS_H

#include "llvm/Bitcode/BitCodes.h"

namespace clang {
namespace index {
namespace store {

static const unsigned STORE_FORMAT_VERSION = 1;

void makeUnitSubDir(SmallVectorImpl<char> &StorePathBuf);
void makeRecordSubDir(SmallVectorImpl<char> &StorePathBuf);

enum BitRecord {
  REC_VERSION,
  REC_DECLINFO,
  REC_DECLOFFSETS,
  REC_DECLOCCURRENCE,
};

enum BitBlock {
  VERSION_BLOCK_ID = llvm::bitc::FIRST_APPLICATION_BLOCKID,
  DECLS_BLOCK_ID,
  DECLOFFSETS_BLOCK_ID,
  DECLOCCURRENCES_BLOCK_ID,
};

typedef SmallVector<uint64_t, 64> RecordData;
typedef SmallVectorImpl<uint64_t> RecordDataImpl;

} // end namespace store
} // end namespace index
} // end namespace clang

#endif
