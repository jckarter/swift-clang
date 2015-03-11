//===--- AST/ModuleProvider.h - Emit and read .pcm files --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_MODULE_PROVIDER_H
#define LLVM_CLANG_AST_MODULE_PROVIDER_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/MemoryBuffer.h"
#include <memory>

namespace llvm {
class raw_ostream;
class BitstreamReader;
}

namespace clang {

class ASTConsumer;
class CodeGenOptions;
class TargetOptions;
class LangOptions;
class DiagnosticsEngine;

class ModuleProvider {
public:
  virtual ~ModuleProvider();
  /// \brief Return an ASTconsumer that can be chained with a
  /// PCHGenerator that takes care of storing the physical
  /// representation of a module.
  virtual std::unique_ptr<ASTConsumer> CreateModuleContainerGenerator(
      DiagnosticsEngine &Diags, const std::string &ModuleName,
      const CodeGenOptions &CGO, const TargetOptions &TO, const LangOptions &LO,
      llvm::raw_ostream *OS,
      std::shared_ptr<std::pair<bool, llvm::SmallVector<char, 0>>> Buffer)
      const = 0;

  /// \brief Initialize an llvm::BitstreamReader with the module
  /// inside the module container Buffer.
  virtual void UnwrapModuleContainer(llvm::MemoryBufferRef Buffer,
                                     llvm::BitstreamReader &StreamFile) const = 0;
};
}

#endif
