//===--- IndexStoreUtils.h - Utilities for working with indexstore.h ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_CINDEXTEST_INDEXSTOREUTILS_H
#define LLVM_CLANG_TOOLS_CINDEXTEST_INDEXSTOREUTILS_H

#include "clang/Basic/LLVM.h"
#include "clang/Index/IndexSymbol.h"
#include "indexstore/indexstore.h"

namespace clang {
namespace index {

/// Map an indexstore_symbol_kind_t to a SymbolKind, handling unknown values.
SymbolKind getSymbolKind(indexstore_symbol_kind_t K);

/// Map an indexstore_symbol_sub_kind_t to a SymbolCXXTemplateKind, handling
/// unknown values.
SymbolCXXTemplateKind getSymbolCXXTemplateKind(indexstore_symbol_sub_kind_t K);

/// Map an indexstore_symbol_language_t to a SymbolLanguage, handling unknown
/// values.
SymbolLanguage getSymbolLanguage(indexstore_symbol_language_t L);

} // end namespace index
} // end namespace clang

#endif
