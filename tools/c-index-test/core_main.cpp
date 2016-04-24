//===-- core_main.cpp - Core Index Tool testbed ---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "IndexStoreUtils.h"
#include "JSONAggregation.h"
#include "indexstore/IndexStoreCXX.h"
#include "clang/Frontend/ASTUnit.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Index/IndexingAction.h"
#include "clang/Index/IndexDataConsumer.h"
#include "clang/Index/IndexRecordReader.h"
#include "clang/Index/IndexUnitReader.h"
#include "clang/Index/USRGeneration.h"
#include "clang/Index/CodegenNameGenerator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/PrettyStackTrace.h"

using namespace clang;
using namespace clang::index;
using namespace llvm;

extern "C" int indextest_core_main(int argc, const char **argv);

namespace {

enum class ActionType {
  None,
  PrintSourceSymbols,
  PrintRecord,
  PrintUnit,
  AggregateAsJSON,
};

namespace options {

static cl::OptionCategory IndexTestCoreCategory("index-test-core options");

static cl::opt<ActionType>
Action(cl::desc("Action:"), cl::init(ActionType::None),
       cl::values(
          clEnumValN(ActionType::PrintSourceSymbols,
                     "print-source-symbols", "Print symbols from source"),
          clEnumValN(ActionType::PrintRecord,
                     "print-record", "Print record info"),
          clEnumValN(ActionType::PrintUnit,
                     "print-unit", "Print unit info"),
          clEnumValN(ActionType::AggregateAsJSON,
                     "aggregate-json", "Aggregate index data in JSON format"),
          clEnumValEnd),
       cl::cat(IndexTestCoreCategory));

static cl::opt<std::string>
OutputFile("o", cl::desc("output file"),
           cl::cat(IndexTestCoreCategory));

static cl::list<std::string>
InputFiles(cl::Positional, cl::desc("<filename>..."));

static cl::extrahelp MoreHelp(
  "\nAdd \"-- <compiler arguments>\" at the end to setup the compiler "
  "invocation\n"
);

}
} // anonymous namespace

static void printSymbolInfo(SymbolInfo SymInfo, raw_ostream &OS);
static void printSymbolNameAndUSR(const Decl *D, ASTContext &Ctx,
                                  raw_ostream &OS);

namespace {

class PrintIndexDataConsumer : public IndexDataConsumer {
  raw_ostream &OS;
  std::unique_ptr<CodegenNameGenerator> CGNameGen;

public:
  PrintIndexDataConsumer(raw_ostream &OS) : OS(OS) {
  }

  void initialize(ASTContext &Ctx) override {
    CGNameGen.reset(new CodegenNameGenerator(Ctx));
  }

  bool handleDeclOccurence(const Decl *D, SymbolRoleSet Roles,
                           ArrayRef<SymbolRelation> Relations,
                           FileID FID, unsigned Offset,
                           ASTNodeInfo ASTNode) override {
    ASTContext &Ctx = D->getASTContext();
    SourceManager &SM = Ctx.getSourceManager();

    unsigned Line = SM.getLineNumber(FID, Offset);
    unsigned Col = SM.getColumnNumber(FID, Offset);
    OS << Line << ':' << Col << " | ";

    printSymbolInfo(getSymbolInfo(D), OS);
    OS << " | ";

    printSymbolNameAndUSR(D, Ctx, OS);
    OS << " | ";

    if (CGNameGen->writeName(D, OS))
      OS << "<no-cgname>";
    OS << " | ";

    printSymbolRoles(Roles, OS);
    OS << " | ";

    OS << "rel: " << Relations.size() << '\n';

    for (auto &SymRel : Relations) {
      OS << '\t';
      printSymbolRoles(SymRel.Roles, OS);
      OS << " | ";
      printSymbolNameAndUSR(SymRel.RelatedSymbol, Ctx, OS);
      OS << '\n';
    }

    return true;
  }

  bool handleModuleOccurence(const ImportDecl *ImportD, SymbolRoleSet Roles,
                             FileID FID, unsigned Offset) override {
    ASTContext &Ctx = ImportD->getASTContext();
    SourceManager &SM = Ctx.getSourceManager();

    unsigned Line = SM.getLineNumber(FID, Offset);
    unsigned Col = SM.getColumnNumber(FID, Offset);
    OS << Line << ':' << Col << " | ";

    printSymbolInfo(getSymbolInfo(ImportD), OS);
    OS << " | ";

    OS << ImportD->getImportedModule()->getFullModuleName() << " | ";

    printSymbolRoles(Roles, OS);
    OS << " |\n";

    return true;
  }
};

} // anonymous namespace

//===----------------------------------------------------------------------===//
// Print Source Symbols
//===----------------------------------------------------------------------===//

static bool printSourceSymbols(ArrayRef<const char *> Args) {
  SmallVector<const char *, 4> ArgsWithProgName;
  ArgsWithProgName.push_back("clang");
  ArgsWithProgName.append(Args.begin(), Args.end());
  IntrusiveRefCntPtr<DiagnosticsEngine>
    Diags(CompilerInstance::createDiagnostics(new DiagnosticOptions));
  IntrusiveRefCntPtr<CompilerInvocation>
    CInvok(createInvocationFromCommandLine(ArgsWithProgName, Diags));
  if (!CInvok)
    return true;

  auto DataConsumer = std::make_shared<PrintIndexDataConsumer>(outs());
  IndexingOptions IndexOpts;
  std::unique_ptr<FrontendAction> IndexAction;
  IndexAction = createIndexingAction(DataConsumer, IndexOpts,
                                     /*WrappedAction=*/nullptr);

  auto PCHContainerOps = std::make_shared<PCHContainerOperations>();
  ASTUnit *Unit =
   ASTUnit::LoadFromCompilerInvocationAction(CInvok.get(), PCHContainerOps,
                                             Diags, IndexAction.get());

  if (!Unit)
    return true;

  return false;
}

#if INDEXSTORE_HAS_BLOCKS

//===----------------------------------------------------------------------===//
// Print Record
//===----------------------------------------------------------------------===//

static void printSymbol(const IndexRecordDecl &Rec, raw_ostream &OS);
static void printSymbol(const IndexRecordOccurrence &Rec, raw_ostream &OS);

static int printRecord(StringRef Filename, raw_ostream &OS) {
  std::string Error;
  auto Reader = IndexRecordReader::createWithFilePath(Filename, Error);
  if (!Reader) {
    errs() << Error << '\n';
    return true;
  }

  Reader->foreachDecl(/*noCache=*/true, [&](const IndexRecordDecl *Rec)->bool {
    printSymbol(*Rec, OS);
    return true;
  });
  OS << "------------\n";
  Reader->foreachOccurrence([&](const IndexRecordOccurrence &Rec)->bool {
    printSymbol(Rec, OS);
    return true;
  });

  return false;
};

//===----------------------------------------------------------------------===//
// Print Store Records
//===----------------------------------------------------------------------===//

static void printSymbol(indexstore::IndexRecordSymbol Sym, raw_ostream &OS);
static void printSymbol(indexstore::IndexRecordOccurrence Occur, raw_ostream &OS);

static bool printStoreRecord(indexstore::IndexStore &Store, StringRef RecName,
                             StringRef FilePath, raw_ostream &OS) {
  std::string Error;
  indexstore::IndexRecordReader Reader(Store, RecName, Error);
  if (!Reader) {
    errs() << "error loading record: " << Error << "\n";
    return true;
  }

  StringRef Filename = sys::path::filename(FilePath);
  OS << Filename << '\n';
  OS << "------------\n";
  Reader.foreachSymbol(/*noCache=*/true, [&](indexstore::IndexRecordSymbol Sym) -> bool {
    printSymbol(Sym, OS);
    return true;
  });
  OS << "------------\n";
  Reader.foreachOccurrence([&](indexstore::IndexRecordOccurrence Occur)->bool {
    printSymbol(Occur, OS);
    return true;
  });

  return false;
}

static int printStoreRecords(StringRef StorePath, raw_ostream &OS) {
  std::string Error;
  indexstore::IndexStore Store(StorePath, Error);
  if (!Store) {
    errs() << "error loading store: " << Error << "\n";
    return 1;
  }

  bool Success = Store.foreachUnit(/*sorted=*/true, [&](StringRef UnitName) -> bool {
    indexstore::IndexUnitReader Reader(Store, UnitName, Error);
    if (!Reader) {
      errs() << "error loading unit: " << Error << "\n";
      return false;
    }
    return Reader.foreachDependency([&](indexstore::IndexUnitDependency Dep) -> bool {
      if (Dep.getKind() == indexstore::IndexUnitDependency::DependencyKind::Record) {
        bool Err = printStoreRecord(Store, Dep.getName(), Dep.getFilePath(), OS);
        OS << '\n';
        return !Err;
      }
      return true;
    });
  });

  return !Success;
}


//===----------------------------------------------------------------------===//
// Print Unit
//===----------------------------------------------------------------------===//

static int printUnit(StringRef Filename, raw_ostream &OS) {
  std::string Error;
  auto Reader = IndexUnitReader::createWithFilePath(Filename, Error);
  if (!Reader) {
    errs() << Error << '\n';
    return true;
  }

  OS << "work-dir: " << Reader->getWorkingDirectory() << '\n';
  OS << "out-file: " << Reader->getOutputFile() << '\n';
  OS << "target: " << Reader->getTarget() << '\n';
  OS << "DEPEND START: " << Reader->getDependencyFiles().size() << '\n';
  Reader->foreachDependency([&](bool isUnit, StringRef unitOrRecordName,
                                StringRef Filename, unsigned depIndex) -> bool {
    if (isUnit)
      OS << "Unit | ";
    else
      OS << "Record | ";
    OS << Filename << " | " << unitOrRecordName << '\n';
    return true;
  });
  OS << "DEPEND END\n";

  return false;
};

//===----------------------------------------------------------------------===//
// Print Store Units
//===----------------------------------------------------------------------===//

static bool printStoreUnit(indexstore::IndexStore &Store, StringRef UnitName,
                           raw_ostream &OS) {
  std::string Error;
  indexstore::IndexUnitReader Reader(Store, UnitName, Error);
  if (!Reader) {
    errs() << "error loading unit: " << Error << "\n";
    return true;
  }

  OS << "work-dir: " << Reader.getWorkingDirectory() << '\n';
  OS << "out-file: " << Reader.getOutputFile() << '\n';
  OS << "target: " << Reader.getTarget() << '\n';
  OS << "DEPEND START: " << Reader.getDependenciesCount() << '\n';
  Reader.foreachDependency([&](indexstore::IndexUnitDependency Dep) -> bool {
    if (Dep.getKind() == indexstore::IndexUnitDependency::DependencyKind::Unit)
      OS << "Unit | ";
    else
      OS << "Record | ";
    OS << Dep.getFilePath() << " | " << Dep.getName() << '\n';
    return true;
  });
  OS << "DEPEND END\n";

  return false;
}

static int printStoreUnits(StringRef StorePath, raw_ostream &OS) {
  std::string Error;
  indexstore::IndexStore Store(StorePath, Error);
  if (!Store) {
    errs() << "error loading store: " << Error << "\n";
    return 1;
  }

  bool Success = Store.foreachUnit(/*sorted=*/true, [&](StringRef UnitName) -> bool {
    OS << UnitName << '\n';
    OS << "--------\n";
    bool err = printStoreUnit(Store, UnitName, OS);
    OS << '\n';
    return !err;
  });

  return !Success;
}


#else

static int printUnit(StringRef Filename, raw_ostream &OS) {
  return 1;
}

static int printStoreUnits(StringRef StorePath, raw_ostream &OS) {
  return 1;
}

#endif

//===----------------------------------------------------------------------===//
// Helper Utils
//===----------------------------------------------------------------------===//

static void printSymbolInfo(SymbolInfo SymInfo, raw_ostream &OS) {
  OS << getSymbolKindString(SymInfo.Kind);
  if (SymInfo.SubKinds) {
    OS << '(';
    printSymbolSubKinds(SymInfo.SubKinds, OS);
    OS << ')';
  }
  OS << '/' << getSymbolLanguageString(SymInfo.Lang);
}

static void printSymbolNameAndUSR(const Decl *D, ASTContext &Ctx,
                                  raw_ostream &OS) {
  if (printSymbolName(D, Ctx.getLangOpts(), OS)) {
    OS << "<no-name>";
  }
  OS << " | ";

  SmallString<256> USRBuf;
  if (generateUSRForDecl(D, USRBuf)) {
    OS << "<no-usr>";
  } else {
    OS << USRBuf;
  }
}

#if INDEXSTORE_HAS_BLOCKS

static void printSymbol(const IndexRecordDecl &Rec, raw_ostream &OS) {
  SymbolInfo SymInfo{ Rec.Kind, Rec.SubKinds, Rec.Lang };
  printSymbolInfo(SymInfo, OS);
  OS << " | ";

  if (Rec.Name.empty())
    OS << "<no-name>";
  else
    OS << Rec.Name;
  OS << " | ";

  if (Rec.USR.empty())
    OS << "<no-usr>";
  else
    OS << Rec.USR;
  OS << " | ";

  if (Rec.CodeGenName.empty())
    OS << "<no-cgname>";
  else
    OS << Rec.CodeGenName;
  OS << " | ";

  printSymbolRoles(Rec.Roles, OS);
  OS << " - ";
  printSymbolRoles(Rec.RelatedRoles, OS);
  OS << '\n';
}

static void printSymbol(const IndexRecordOccurrence &Rec, raw_ostream &OS) {
  OS << Rec.Line << ':' << Rec.Column << " | ";
  SymbolInfo SymInfo{ Rec.Dcl->Kind, Rec.Dcl->SubKinds, Rec.Dcl->Lang };
  printSymbolInfo(SymInfo, OS);
  OS << " | ";

  if (Rec.Dcl->USR.empty())
    OS << "<no-usr>";
  else
    OS << Rec.Dcl->USR;
  OS << " | ";

  printSymbolRoles(Rec.Roles, OS);
  OS << " | ";
  OS << "rel: " << Rec.Relations.size() << '\n';
  for (auto &Rel : Rec.Relations) {
    OS << '\t';
    printSymbolRoles(Rel.Roles, OS);
    OS << " | ";
    if (Rel.Dcl->USR.empty())
      OS << "<no-usr>";
    else
      OS << Rel.Dcl->USR;
    OS << '\n';
  }
}

/// Map an indexstore_symbol_kind_t to a SymbolKind, handling unknown values.
SymbolKind index::getSymbolKind(indexstore_symbol_kind_t K) {
  switch ((uint64_t)K) {
  default:
  case INDEXSTORE_SYMBOL_KIND_UNKNOWN:
    return SymbolKind::Unknown;
  case INDEXSTORE_SYMBOL_KIND_MODULE:
    return SymbolKind::Module;
  case INDEXSTORE_SYMBOL_KIND_NAMESPACE:
    return SymbolKind::Namespace;
  case INDEXSTORE_SYMBOL_KIND_NAMESPACEALIAS:
    return SymbolKind::NamespaceAlias;
  case INDEXSTORE_SYMBOL_KIND_MACRO:
    return SymbolKind::Macro;
  case INDEXSTORE_SYMBOL_KIND_ENUM:
    return SymbolKind::Enum;
  case INDEXSTORE_SYMBOL_KIND_STRUCT:
    return SymbolKind::Struct;
  case INDEXSTORE_SYMBOL_KIND_CLASS:
    return SymbolKind::Class;
  case INDEXSTORE_SYMBOL_KIND_PROTOCOL:
    return SymbolKind::Protocol;
  case INDEXSTORE_SYMBOL_KIND_EXTENSION:
    return SymbolKind::Extension;
  case INDEXSTORE_SYMBOL_KIND_UNION:
    return SymbolKind::Union;
  case INDEXSTORE_SYMBOL_KIND_TYPEALIAS:
    return SymbolKind::TypeAlias;
  case INDEXSTORE_SYMBOL_KIND_FUNCTION:
    return SymbolKind::Function;
  case INDEXSTORE_SYMBOL_KIND_VARIABLE:
    return SymbolKind::Variable;
  case INDEXSTORE_SYMBOL_KIND_FIELD:
    return SymbolKind::Field;
  case INDEXSTORE_SYMBOL_KIND_ENUMCONSTANT:
    return SymbolKind::EnumConstant;
  case INDEXSTORE_SYMBOL_KIND_INSTANCEMETHOD:
    return SymbolKind::InstanceMethod;
  case INDEXSTORE_SYMBOL_KIND_CLASSMETHOD:
    return SymbolKind::ClassMethod;
  case INDEXSTORE_SYMBOL_KIND_STATICMETHOD:
    return SymbolKind::StaticMethod;
  case INDEXSTORE_SYMBOL_KIND_INSTANCEPROPERTY:
    return SymbolKind::InstanceProperty;
  case INDEXSTORE_SYMBOL_KIND_CLASSPROPERTY:
    return SymbolKind::ClassProperty;
  case INDEXSTORE_SYMBOL_KIND_STATICPROPERTY:
    return SymbolKind::StaticProperty;
  case INDEXSTORE_SYMBOL_KIND_CONSTRUCTOR:
    return SymbolKind::Constructor;
  case INDEXSTORE_SYMBOL_KIND_DESTRUCTOR:
    return SymbolKind::Destructor;
  case INDEXSTORE_SYMBOL_KIND_CONVERSIONFUNCTION:
    return SymbolKind::ConversionFunction;
  }
}

/// Map an indexstore_symbol_language_t to a SymbolLanguage, handling unknown
/// values.
SymbolLanguage index::getSymbolLanguage(indexstore_symbol_language_t L) {
  switch ((uint64_t)L) {
  default: // FIXME: add an unknown language?
  case INDEXSTORE_SYMBOL_LANG_C:
    return SymbolLanguage::C;
  case INDEXSTORE_SYMBOL_LANG_OBJC:
    return SymbolLanguage::ObjC;
  case INDEXSTORE_SYMBOL_LANG_CXX:
    return SymbolLanguage::CXX;
  }
}

static void printSymbol(indexstore::IndexRecordSymbol Sym, raw_ostream &OS) {
  SymbolInfo SymInfo{getSymbolKind(Sym.getKind()),
                     SymbolSubKindSet(Sym.getSubKinds()),
                     getSymbolLanguage(Sym.getLanguage())};

  printSymbolInfo(SymInfo, OS);
  OS << " | ";

  if (Sym.getName().empty())
    OS << "<no-name>";
  else
    OS << Sym.getName();
  OS << " | ";

  if (Sym.getUSR().empty())
    OS << "<no-usr>";
  else
    OS << Sym.getUSR();
  OS << " | ";

  if (Sym.getCodegenName().empty())
    OS << "<no-cgname>";
  else
    OS << Sym.getCodegenName();
  OS << " | ";

  printSymbolRoles(Sym.getRoles(), OS);
  OS << " - ";
  printSymbolRoles(Sym.getRelatedRoles(), OS);
  OS << '\n';
}

static void printSymbol(indexstore::IndexRecordOccurrence Occur, raw_ostream &OS) {
  OS << Occur.getLineCol().first << ':' << Occur.getLineCol().second << " | ";
  auto Sym = Occur.getSymbol();
  SymbolInfo SymInfo{getSymbolKind(Sym.getKind()),
                     SymbolSubKindSet(Sym.getSubKinds()),
                     getSymbolLanguage(Sym.getLanguage())};

  printSymbolInfo(SymInfo, OS);
  OS << " | ";

  if (Sym.getUSR().empty())
    OS << "<no-usr>";
  else
    OS << Sym.getUSR();
  OS << " | ";

  unsigned NumRelations = 0;
  Occur.foreachRelation([&](indexstore::IndexSymbolRelation) {
    ++NumRelations;
    return true;
  });

  printSymbolRoles(Occur.getRoles(), OS);
  OS << " | ";
  OS << "rel: " << NumRelations << '\n';
  Occur.foreachRelation([&](indexstore::IndexSymbolRelation Rel) {
    OS << '\t';
    printSymbolRoles(Rel.getRoles(), OS);
    OS << " | ";
    auto Sym = Rel.getSymbol();
    if (Sym.getUSR().empty())
      OS << "<no-usr>";
    else
      OS << Sym.getUSR();
    OS << '\n';
    return true;
  });
}

#else

static int printRecord(StringRef Filename, raw_ostream &OS) {
  return 1;
}
static int printStoreRecords(StringRef StorePath, raw_ostream &OS) {
  return 1;
}

#endif
//===----------------------------------------------------------------------===//
// Command line processing.
//===----------------------------------------------------------------------===//

int indextest_core_main(int argc, const char **argv) {
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);

  std::vector<const char *> CompArgs;
  const char **DoubleDash = std::find(argv, argv + argc, StringRef("--"));
  if (DoubleDash != argv + argc) {
    CompArgs = std::vector<const char *>(DoubleDash + 1, argv + argc);
    argc = DoubleDash - argv;
  }

  cl::HideUnrelatedOptions(options::IndexTestCoreCategory);
  cl::ParseCommandLineOptions(argc, argv, "index-test-core");

  if (options::Action == ActionType::None) {
    errs() << "error: action required; pass '-help' for options\n";
    return 1;
  }

  if (options::Action == ActionType::PrintSourceSymbols) {
    if (CompArgs.empty()) {
      errs() << "error: missing compiler args; pass '-- <compiler arguments>'\n";
      return 1;
    }
    return printSourceSymbols(CompArgs);
  }

  if (options::Action == ActionType::PrintRecord) {
    if (options::InputFiles.empty()) {
      errs() << "error: missing input file or directory\n";
      return 1;
    }

    if (sys::fs::is_directory(options::InputFiles[0]))
      return printStoreRecords(options::InputFiles[0], outs());
    else
      return printRecord(options::InputFiles[0], outs());
  }

  if (options::Action == ActionType::PrintUnit) {
    if (options::InputFiles.empty()) {
      errs() << "error: missing input file or directory\n";
      return 1;
    }

    if (sys::fs::is_directory(options::InputFiles[0]))
      return printStoreUnits(options::InputFiles[0], outs());
    else
      return printUnit(options::InputFiles[0], outs());
  }

  if (options::Action == ActionType::AggregateAsJSON) {
    if (options::InputFiles.empty()) {
      errs() << "error: missing input data store directory\n";
      return 1;
    }
    StringRef storePath = options::InputFiles[0];
    if (options::OutputFile.empty())
      return aggregateDataAsJSON(storePath, outs());
    std::error_code EC;
    raw_fd_ostream OS(options::OutputFile, EC, llvm::sys::fs::F_None);
    if (EC) {
      errs() << "failed to open output file: " << EC.message() << '\n';
      return 1;
    }
    return aggregateDataAsJSON(storePath, OS);
  }

  return 0;
}
