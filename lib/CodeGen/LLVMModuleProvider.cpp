//===--- CodeGenModuleContainer.cpp - Emit .pcm files ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/CodeGen/LLVMModuleProvider.h"
#include "CodeGenModule.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/CodeGen/BackendUtil.h"
#include "clang/Frontend/CodeGenOptions.h"
#include "clang/Serialization/ASTWriter.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Bitcode/BitstreamReader.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Object/COFF.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/TargetRegistry.h"
#include <memory>
using namespace clang;

namespace {
class ModuleContainerGenerator : public ASTConsumer {
  DiagnosticsEngine &Diags;
  std::unique_ptr<const llvm::DataLayout> TD;
  ASTContext *Ctx;
  const CodeGenOptions CodeGenOpts;
  const TargetOptions TargetOpts;
  const LangOptions LangOpts;
  llvm::LLVMContext VMContext;
  std::unique_ptr<llvm::Module> M;
  std::unique_ptr<CodeGen::CodeGenModule> Builder;
  raw_ostream *OS;
  std::shared_ptr<std::pair<bool, llvm::SmallVector<char, 0>>> Buffer;

public:
  ModuleContainerGenerator(
      DiagnosticsEngine &diags, const std::string &ModuleName,
      const CodeGenOptions &CGO, const TargetOptions &TO, const LangOptions &LO,
      raw_ostream *OS,
      std::shared_ptr<std::pair<bool, llvm::SmallVector<char, 0>>> Buffer)
      : Diags(diags), CodeGenOpts(CGO), TargetOpts(TO), LangOpts(LO),
        M(new llvm::Module(ModuleName, VMContext)), OS(OS), Buffer(Buffer) {}

  virtual ~ModuleContainerGenerator() {}

  void Initialize(ASTContext &Context) override {
    Ctx = &Context;
    M->setTargetTriple(Ctx->getTargetInfo().getTriple().getTriple());
    M->setDataLayout(Ctx->getTargetInfo().getTargetDescription());
    TD.reset(new llvm::DataLayout(Ctx->getTargetInfo().getTargetDescription()));
    Builder.reset(
        new CodeGen::CodeGenModule(Context, CodeGenOpts, *M, *TD, Diags));
  }

  /// Emit a container holding the serialized AST.
  void HandleTranslationUnit(ASTContext &Ctx) override {
    if (Diags.hasErrorOccurred()) {
      if (Builder)
        Builder->clear();
      M.reset();
      return;
    }

    // Finalize the Builder.
    if (Builder)
      Builder->Release();

    // Initialize the backend if we haven't done so already.
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllAsmPrinters();
    LLVMInitializeAllTargetMCs();

    // Ensure the target exists.
    std::string Error;
    auto Triple = Ctx.getTargetInfo().getTriple();
    if (!llvm::TargetRegistry::lookupTarget(Triple.getTriple(), Error))
      llvm::report_fatal_error(Error);

    // Emit the serialized Clang AST into its own section.
    assert(Buffer->first && "serialization did not complete");
    auto &SerializedAST = Buffer->second;
    auto Size = SerializedAST.size();
    auto Int8Ty = llvm::Type::getInt8Ty(VMContext);
    auto *Ty = llvm::ArrayType::get(Int8Ty, Size);
    auto *Data = llvm::ConstantDataArray::getString(
        VMContext, StringRef(SerializedAST.data(), Size), /*AddNull=*/false);
    auto *ASTSym = new llvm::GlobalVariable(
        *M, Ty, /*constant*/ true, llvm::GlobalVariable::InternalLinkage, Data,
        "__clang_ast");
    ASTSym->setAlignment(8);
    if (Triple.isOSBinFormatMachO())
      // Include Mach-O segment name.
      ASTSym->setSection("__CLANG,__clangast");
    else if (Triple.isOSBinFormatCOFF())
      // Adhere to COFF eight-character limit.
      ASTSym->setSection("clangast");
    else
      ASTSym->setSection("__clangast");

    // Use the LLVM backend to emit the pcm.
    clang::EmitBackendOutput(Diags, CodeGenOpts, TargetOpts, LangOpts,
                             Ctx.getTargetInfo().getTargetDescription(),
                             M.get(), BackendAction::Backend_EmitObj, OS);

    // Make sure the module container hits disk now.
    OS->flush();

    // Free up some memory, in case the process is kept alive.
    SerializedAST.clear();
  }
};
}

std::unique_ptr<ASTConsumer> LLVMModuleProvider::CreateModuleContainerGenerator(
    DiagnosticsEngine &Diags, const std::string &ModuleName,
    const CodeGenOptions &CGO, const TargetOptions &TO, const LangOptions &LO,
    llvm::raw_ostream *OS,
    std::shared_ptr<std::pair<bool, SmallVector<char, 0>>> Buffer) const {
  return llvm::make_unique<ModuleContainerGenerator>(Diags, ModuleName, CGO, TO,
                                                     LO, OS, Buffer);
}

void LLVMModuleProvider::UnwrapModuleContainer(
    llvm::MemoryBufferRef Buffer, llvm::BitstreamReader &StreamFile) const {
  if (auto OF = llvm::object::ObjectFile::createObjectFile(Buffer)) {
    bool IsCOFF = isa<llvm::object::COFFObjectFile>(OF.get().get());
    // Find the clang AST section in the container.
    for (auto &Section : OF->get()->sections()) {
      StringRef Name;
      Section.getName(Name);
      if ((!IsCOFF && Name == "__clangast") || (IsCOFF && Name == "clangast")) {
        StringRef Buf;
        Section.getContents(Buf);
        return StreamFile.init((const unsigned char *)Buf.begin(),
                               (const unsigned char *)Buf.end());
      }
    }
  }
  StreamFile.init((const unsigned char *)Buffer.getBufferStart(),
                  (const unsigned char *)Buffer.getBufferEnd());
}
