#ifndef LLVM_CLANG_CLANGSERVICE_SUPPORT_LIVEDATA_H
#define LLVM_CLANG_CLANGSERVICE_SUPPORT_LIVEDATA_H

#include "clang-service/Service.h"
#include "clang-service/SafeLibclang.h"

namespace ClangService {

struct LiveData {
  Libclang::UniqueCXCodeCompleteResultsPtr CodeCompleteResults;

  LiveData() : CodeCompleteResults(nullptr) {}
};

} // end namespace ClangService

#endif // LLVM_CLANG_CLANGSERVICE_SUPPORT_LIVEDATA_H
