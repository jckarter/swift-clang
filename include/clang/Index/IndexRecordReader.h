//===--- IndexRecordReader.h - Index record deserialization ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_INDEX_INDEXRECORDREADER_H
#define LLVM_CLANG_INDEX_INDEXRECORDREADER_H

#include "clang/Index/IndexSymbol.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include <memory>

namespace llvm {
  class MemoryBuffer;
}

namespace clang {
namespace index {

struct IndexRecordDecl {
  unsigned DeclID;
  SymbolKind Kind;
  SymbolLanguage Lang;
  SymbolCXXTemplateKind CXXTemplateKind;
  SymbolRoleSet Roles;
  SymbolRoleSet RelatedRoles;
  StringRef Name;
  StringRef USR;
  StringRef CodeGenName;
};

struct IndexRecordRelation {
  SymbolRoleSet Roles;
  const IndexRecordDecl *Dcl = nullptr;

  IndexRecordRelation() = default;
  IndexRecordRelation(SymbolRoleSet Roles, const IndexRecordDecl *Dcl)
    : Roles(Roles), Dcl(Dcl) {}
};

struct IndexRecordOccurrence {
  const IndexRecordDecl *Dcl;
  SmallVector<IndexRecordRelation, 4> Relations;
  SymbolRoleSet Roles;
  unsigned Line;
  unsigned Column;
};

class IndexRecordReader {
  IndexRecordReader();

public:
  static std::unique_ptr<IndexRecordReader>
    createWithRecordFilename(StringRef RecordFilename, StringRef StorePath,
                             std::string &Error);
  static std::unique_ptr<IndexRecordReader>
    createWithFilePath(StringRef FilePath, std::string &Error);
  static std::unique_ptr<IndexRecordReader>
    createWithBuffer(std::unique_ptr<llvm::MemoryBuffer> Buffer,
                     std::string &Error);

  ~IndexRecordReader();

  struct DeclSearchReturn {
    bool AcceptDecl;
    bool ContinueSearch;
  };
  typedef DeclSearchReturn(DeclSearchCheck)(const IndexRecordDecl &);

  /// Goes through the decls and populates a vector of record decls, based on
  /// what the given function returns.
  ///
  /// Resulting vector can be used as filter for \c foreachOccurrence. This
  /// allows allocating memory only for the record decls that the caller is
  /// interested in.
  void searchDecls(llvm::function_ref<DeclSearchCheck> Checker,
                   SmallVectorImpl<const IndexRecordDecl *> &FoundDecls);

  bool foreachDecl(llvm::function_ref<bool(const IndexRecordDecl *)> Receiver);

  /// \param DeclsFilter if non-empty indicates the list of decls that we want
  /// to get occurrences for. An empty array indicates that we want occurrences
  /// for all decls.
  /// \param RelatedDeclsFilter Same as \c DeclsFilter but for related decls.
  bool foreachOccurrence(ArrayRef<const IndexRecordDecl *> DeclsFilter,
                         ArrayRef<const IndexRecordDecl *> RelatedDeclsFilter,
              llvm::function_ref<bool(const IndexRecordOccurrence &)> Receiver);
  bool foreachOccurrence(
              llvm::function_ref<bool(const IndexRecordOccurrence &)> Receiver);

  struct Implementation;
private:
  Implementation &Impl;
};

} // namespace index
} // namespace clang

#endif
