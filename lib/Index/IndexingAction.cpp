//===- IndexingAction.cpp - Frontend index action -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Index/IndexingAction.h"
#include "clang/Index/IndexDataConsumer.h"
#include "FileIndexRecord.h"
#include "IndexingContext.h"
#include "IndexRecordWriter.h"
#include "IndexUnitWriter.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/MultiplexConsumer.h"
#include "clang/Frontend/Utils.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang;
using namespace clang::index;

void IndexDataConsumer::_anchor() {}

bool IndexDataConsumer::handleDeclOccurence(const Decl *D, SymbolRoleSet Roles,
                                            ArrayRef<SymbolRelation> Relations,
                                            FileID FID, unsigned Offset,
                                            ASTNodeInfo ASTNode) {
  return true;
}

bool IndexDataConsumer::handleMacroOccurence(const IdentifierInfo *Name,
                                             const MacroInfo *MI, SymbolRoleSet Roles,
                                             FileID FID, unsigned Offset) {
  return true;
}

bool IndexDataConsumer::handleModuleOccurence(const ImportDecl *ImportD,
                                              SymbolRoleSet Roles,
                                              FileID FID, unsigned Offset) {
  return true;
}

namespace {

class IndexASTConsumer : public ASTConsumer {
  IndexingContext &IndexCtx;

public:
  IndexASTConsumer(IndexingContext &IndexCtx)
    : IndexCtx(IndexCtx) {}

protected:
  void Initialize(ASTContext &Context) override {
    IndexCtx.setASTContext(Context);
    IndexCtx.getDataConsumer().initialize(Context);
  }

  bool HandleTopLevelDecl(DeclGroupRef DG) override {
    return IndexCtx.indexDeclGroupRef(DG);
  }

  void HandleInterestingDecl(DeclGroupRef DG) override {
    // Ignore deserialized decls.
  }

  void HandleTopLevelDeclInObjCContainer(DeclGroupRef DG) override {
    IndexCtx.indexDeclGroupRef(DG);
  }

  void HandleTranslationUnit(ASTContext &Ctx) override {
  }
};

class IndexActionBase {
protected:
  std::shared_ptr<IndexDataConsumer> DataConsumer;
  IndexingContext IndexCtx;

  IndexActionBase(std::shared_ptr<IndexDataConsumer> dataConsumer,
                  IndexingOptions Opts)
    : DataConsumer(std::move(dataConsumer)),
      IndexCtx(Opts, *DataConsumer) {}

  std::unique_ptr<IndexASTConsumer> createIndexASTConsumer() {
    return llvm::make_unique<IndexASTConsumer>(IndexCtx);
  }

  void finish() {
    DataConsumer->finish();
  }
};

class IndexAction : public ASTFrontendAction, IndexActionBase {
public:
  IndexAction(std::shared_ptr<IndexDataConsumer> DataConsumer,
              IndexingOptions Opts)
    : IndexActionBase(std::move(DataConsumer), Opts) {}

protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override {
    return createIndexASTConsumer();
  }

  void EndSourceFileAction() override {
    FrontendAction::EndSourceFileAction();
    finish();
  }
};

class WrappingIndexAction : public WrapperFrontendAction, IndexActionBase {
  bool IndexActionFailed = false;

public:
  WrappingIndexAction(std::unique_ptr<FrontendAction> WrappedAction,
                      std::shared_ptr<IndexDataConsumer> DataConsumer,
                      IndexingOptions Opts)
    : WrapperFrontendAction(std::move(WrappedAction)),
      IndexActionBase(std::move(DataConsumer), Opts) {}

protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override;
  void EndSourceFileAction() override;
};

} // anonymous namespace

void WrappingIndexAction::EndSourceFileAction() {
  // Invoke wrapped action's method.
  WrapperFrontendAction::EndSourceFileAction();
  if (!IndexActionFailed)
    finish();
}

std::unique_ptr<ASTConsumer>
WrappingIndexAction::CreateASTConsumer(CompilerInstance &CI, StringRef InFile) {
  auto OtherConsumer = WrapperFrontendAction::CreateASTConsumer(CI, InFile);
  if (!OtherConsumer) {
    IndexActionFailed = true;
    return nullptr;
  }

  std::vector<std::unique_ptr<ASTConsumer>> Consumers;
  Consumers.push_back(std::move(OtherConsumer));
  Consumers.push_back(createIndexASTConsumer());
  return llvm::make_unique<MultiplexConsumer>(std::move(Consumers));
}

std::unique_ptr<FrontendAction>
index::createIndexingAction(std::shared_ptr<IndexDataConsumer> DataConsumer,
                            IndexingOptions Opts,
                            std::unique_ptr<FrontendAction> WrappedAction) {
  if (WrappedAction)
    return llvm::make_unique<WrappingIndexAction>(std::move(WrappedAction),
                                                  std::move(DataConsumer),
                                                  Opts);
  return llvm::make_unique<IndexAction>(std::move(DataConsumer), Opts);
}


static bool topLevelDeclVisitor(void *context, const Decl *D) {
  IndexingContext &IndexCtx = *static_cast<IndexingContext*>(context);
  return IndexCtx.indexTopLevelDecl(D);
}

static void indexTranslationUnit(ASTUnit &Unit, IndexingContext &IndexCtx) {
  Unit.visitLocalTopLevelDecls(&IndexCtx, topLevelDeclVisitor);
}

void index::indexASTUnit(ASTUnit &Unit,
                         std::shared_ptr<IndexDataConsumer> DataConsumer,
                         IndexingOptions Opts) {
  IndexingContext IndexCtx(Opts, *DataConsumer);
  IndexCtx.setASTContext(Unit.getASTContext());
  DataConsumer->initialize(Unit.getASTContext());
  indexTranslationUnit(Unit, IndexCtx);
}

//===----------------------------------------------------------------------===//
// Index Data Recording
//===----------------------------------------------------------------------===//

namespace {

class IndexDataRecorder : public IndexDataConsumer {
  typedef llvm::DenseMap<FileID, std::unique_ptr<FileIndexRecord>> RecordByFileTy;
  RecordByFileTy RecordByFile;

public:
  RecordByFileTy::const_iterator record_begin() const { return RecordByFile.begin(); }
  RecordByFileTy::const_iterator record_end() const { return RecordByFile.end(); }
  bool record_empty() const { return RecordByFile.empty(); }

private:
  bool handleDeclOccurence(const Decl *D, SymbolRoleSet Roles,
                           ArrayRef<SymbolRelation> Relations,
                           FileID FID, unsigned Offset,
                           ASTNodeInfo ASTNode) override {
    FileIndexRecord &Rec = getFileIndexRecord(FID);
    Rec.addDeclOccurence(Roles, Offset, D, Relations);
    return true;
  }

  FileIndexRecord &getFileIndexRecord(FileID FID) {
    auto &Entry = RecordByFile[FID];
    if (!Entry) {
      Entry.reset(new FileIndexRecord(FID));
    }
    return *Entry;
  }
};

class IndexDependencyCollector : public DependencyCollector {
  bool NeedsSystemDependencies;

public:
  explicit IndexDependencyCollector(bool NeedsSystemDependencies)
    : NeedsSystemDependencies(NeedsSystemDependencies) {}

  bool needSystemDependencies() override { return NeedsSystemDependencies; }
};

class IndexRecordActionBase {
protected:
  RecordingOptions RecordOpts;
  IndexDataRecorder Recorder;
  IndexingContext IndexCtx;
  IndexDependencyCollector DepCollector;

  IndexRecordActionBase(IndexingOptions IndexOpts, RecordingOptions recordOpts)
    : RecordOpts(std::move(recordOpts)),
      IndexCtx(IndexOpts, Recorder),
      DepCollector(RecordOpts.RecordSystemDependencies) {}

  std::unique_ptr<IndexASTConsumer>
  createIndexASTConsumer(CompilerInstance &CI) {
    Preprocessor &PP = CI.getPreprocessor();
    DepCollector.attachToPreprocessor(PP);

    return llvm::make_unique<IndexASTConsumer>(IndexCtx);
  }

  void finish(CompilerInstance &CI);
};

class IndexRecordAction : public ASTFrontendAction, IndexRecordActionBase {
public:
  IndexRecordAction(IndexingOptions IndexOpts, RecordingOptions RecordOpts)
    : IndexRecordActionBase(std::move(IndexOpts), std::move(RecordOpts)) {}

protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override {
    return createIndexASTConsumer(CI);
  }

  void EndSourceFileAction() override {
    FrontendAction::EndSourceFileAction();
    finish(getCompilerInstance());
  }
};

class WrappingIndexRecordAction : public WrapperFrontendAction, IndexRecordActionBase {
  bool IndexActionFailed = false;

public:
  WrappingIndexRecordAction(std::unique_ptr<FrontendAction> WrappedAction,
                            IndexingOptions IndexOpts,
                            RecordingOptions RecordOpts)
    : WrapperFrontendAction(std::move(WrappedAction)),
      IndexRecordActionBase(std::move(IndexOpts), std::move(RecordOpts)) {}

protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override {
    auto OtherConsumer = WrapperFrontendAction::CreateASTConsumer(CI, InFile);
    if (!OtherConsumer) {
      IndexActionFailed = true;
      return nullptr;
    }

    std::vector<std::unique_ptr<ASTConsumer>> Consumers;
    Consumers.push_back(std::move(OtherConsumer));
    Consumers.push_back(createIndexASTConsumer(CI));
    return llvm::make_unique<MultiplexConsumer>(std::move(Consumers));
  }

  void EndSourceFileAction() override {
    // Invoke wrapped action's method.
    WrapperFrontendAction::EndSourceFileAction();
    if (!IndexActionFailed)
      finish(getCompilerInstance());
  }
};

} // anonymous namespace

void IndexRecordActionBase::finish(CompilerInstance &CI) {
  SourceManager &SM = CI.getSourceManager();
  DiagnosticsEngine &Diag = CI.getDiagnostics();
  StringRef ArenaPath = RecordOpts.DataDirPath;

  using namespace llvm::sys;
  std::string SubPath = ArenaPath;
  SubPath += "/clang/records";
  std::error_code EC = fs::create_directories(SubPath);
  if (EC) {
    unsigned DiagID = Diag.getCustomDiagID(DiagnosticsEngine::Error,
                                          "failed creating directory '%0': %1");
    Diag.Report(DiagID) << SubPath << EC.message();
    return;
  }
  SubPath = ArenaPath;
  SubPath += "/clang/units";
  EC = fs::create_directory(SubPath);
  if (EC) {
    unsigned DiagID = Diag.getCustomDiagID(DiagnosticsEngine::Error,
                                           "failed creating directory '%0': %1");
    Diag.Report(DiagID) << SubPath << EC.message();
    return;
  }

  std::string OutputFile = CI.getFrontendOpts().OutputFile;
  if (OutputFile.empty()) {
    OutputFile = CI.getFrontendOpts().Inputs[0].getFile();
    OutputFile += ".o";
  }

  IndexUnitWriter UnitWriter(ArenaPath, OutputFile, CI.getTargetOpts().Triple);

  FileManager &FileMgr = SM.getFileManager();
  for (auto &Dep : DepCollector.getDependencies()) {
    if (const FileEntry *FE = FileMgr.getFile(Dep))
      UnitWriter.addDependency(FE);
  }

  IndexRecordWriter RecordWriter(CI.getASTContext(), RecordOpts);
  for (auto I = Recorder.record_begin(), E = Recorder.record_end(); I != E; ++I) {
    FileID FID = I->first;
    const FileIndexRecord &Rec = *I->second;
    const FileEntry *FE = SM.getFileEntryForID(FID);
    std::string RecordFile;
    std::string Error;

    if (RecordWriter.writeRecord(FE->getName(), Rec, Error, &RecordFile)) {
      unsigned DiagID = Diag.getCustomDiagID(DiagnosticsEngine::Error,
                                             "failed writing record '%0': %1");
      Diag.Report(DiagID) << RecordFile << Error;
      return;
    }
    UnitWriter.addRecordFile(RecordFile, FE);
  }

  std::string Error;
  if (UnitWriter.write(Error)) {
    unsigned DiagID = Diag.getCustomDiagID(DiagnosticsEngine::Error,
                                           "failed writing unit data: %0");
    Diag.Report(DiagID) << Error;
    return;
  }
}

std::unique_ptr<FrontendAction>
index::createIndexDataRecordingAction(IndexingOptions IndexOpts,
                                      RecordingOptions RecordOpts,
                                std::unique_ptr<FrontendAction> WrappedAction) {
  if (WrappedAction)
    return llvm::make_unique<WrappingIndexRecordAction>(std::move(WrappedAction),
                                                        std::move(IndexOpts),
                                                        std::move(RecordOpts));
  return llvm::make_unique<IndexRecordAction>(std::move(IndexOpts),
                                              std::move(RecordOpts));
}
