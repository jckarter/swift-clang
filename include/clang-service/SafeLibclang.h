#ifndef LLVM_CLANG_CLANGSERVICE_SAFELIBCLANG_H
#define LLVM_CLANG_CLANGSERVICE_SAFELIBCLANG_H

#include "clang-c/Index.h"
#include "clang-service/Support/OwnedString.h"

#include "llvm/ADT/SmallVector.h"

namespace Libclang {

template<typename T, void (*Destructor)(T)>
class CXUnique {
  T Obj;

public:
  CXUnique(T Obj) : Obj(Obj) {}

  ~CXUnique() { Destructor(Obj); }

  operator T() const { return Obj; }

  T operator->() const { return Obj; }

  T *storage() { return &Obj; }

  CXUnique(const CXUnique &) = delete;

  CXUnique &operator=(const CXUnique &) = delete;

  CXUnique(CXUnique &&Other) { operator=(std::move(Other)); }

  CXUnique &operator=(CXUnique &&Other) {
    Obj = Other.Obj;
    Other.Obj = {};
    return *this;
  }
};

using UniqueCXIndex = CXUnique<CXIndex, clang_disposeIndex>;

using UniqueCXTranslationUnit =
    CXUnique<CXTranslationUnit, clang_disposeTranslationUnit>;

using UniqueCXCodeCompleteResultsPtr =
    CXUnique<CXCodeCompleteResults *, clang_disposeCodeCompleteResults>;

using UniqueCXIndexAction = CXUnique<CXIndexAction, clang_IndexAction_dispose>;

using UniqueCXString = CXUnique<CXString, clang_disposeString>;

using UniqueCXDiagnostic = CXUnique<CXDiagnostic, clang_disposeDiagnostic>;

class SafeCXIndex : public UniqueCXIndex {
  /// A CXIndex object cannot be freed before all of its associated
  /// objects are freed. Enforce this by storing these objects within
  /// a container index.
  llvm::SmallVector<UniqueCXTranslationUnit, 4> TUs;
  llvm::SmallVector<UniqueCXIndexAction, 4> Actions;

public:
  SafeCXIndex(CXIndex Obj) : UniqueCXIndex(Obj) {}

  SafeCXIndex(SafeCXIndex &&Other) : UniqueCXIndex(std::move(Other)) {
    TUs = std::move(Other.TUs);
  }

  SafeCXIndex &operator=(SafeCXIndex &&Other) {
    UniqueCXIndex::operator=(std::move(Other));
    TUs = std::move(Other.TUs);
    return *this;
  }

  UniqueCXTranslationUnit &
  parseTranslationUnit(const char *Filename, const char *const *CmdArgv,
                       unsigned CmdArgc, struct CXUnsavedFile *UnsavedFiles,
                       unsigned NumUnsavedFiles, unsigned ParseOpts,
                       enum CXErrorCode &Err) {
    UniqueCXTranslationUnit TU{/*unsafe!=*/nullptr};
    Err = clang_parseTranslationUnit2(*this, Filename, CmdArgv, CmdArgc,
                                      UnsavedFiles, NumUnsavedFiles, ParseOpts,
                                      TU.storage());
    TUs.push_back(std::move(TU));
    return TUs.back();
  }

  UniqueCXIndexAction &createIndexAction() {
    UniqueCXIndexAction Act{clang_IndexAction_create(*this)};
    Actions.push_back(std::move(Act));
    return Actions.back();
  }

  UniqueCXTranslationUnit &getTU(unsigned Idx = 0) { return TUs[Idx]; }
};

class OwnedCXString : public ClangService::OwnedString {
  UniqueCXString Str;

public:
  // FIXME: libclang should provide an O(1) clang_getStringLength() API!
  explicit OwnedCXString(UniqueCXString S)
      : ClangService::OwnedString({clang_getCString(S)}), Str(std::move(S)) {}

  virtual ~OwnedCXString() {}
};

} // end namespace Libclang

#endif // LLVM_CLANG_CLANGSERVICE_SAFELIBCLANG_H
