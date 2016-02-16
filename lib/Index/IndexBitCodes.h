//===--- IndexBitCodes.h - Index record common for de/serialization -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_INDEX_INDEXBITCODES_H
#define LLVM_CLANG_LIB_INDEX_INDEXBITCODES_H

#include "llvm/Bitcode/BitCodes.h"

namespace clang {
namespace index {

typedef SmallVector<uint64_t, 64> RecordData;
typedef SmallVectorImpl<uint64_t> RecordDataImpl;

namespace record {

static const unsigned VERSION_MAJOR = 1;
static const unsigned VERSION_MINOR = 0;

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

} // end namespace record
} // end namespace index
} // end namespace clang

#endif
