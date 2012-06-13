//===- TableGen.cpp - Top-Level TableGen implementation for Clang ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the main function for Clang's TableGen.
//
//===----------------------------------------------------------------------===//

#include "TableGenBackends.h" // Declares all backends.

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/TableGen/Error.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenAction.h"

using namespace llvm;
using namespace clang;

enum ActionType {
  GenClangAttrClasses,
  GenClangAttrImpl,
  GenClangAttrList,
  GenClangAttrPCHRead,
  GenClangAttrPCHWrite,
  GenClangAttrSpellingList,
  GenClangAttrLateParsedList,
  GenClangAttrTemplateInstantiate,
  GenClangAttrParsedAttrList,
  GenClangAttrParsedAttrKinds,
  GenClangDiagsDefs,
  GenClangDiagGroups,
  GenClangDiagsIndexName,
  GenClangDeclNodes,
  GenClangStmtNodes,
  GenClangSACheckers,
  GenOptParserDefs, GenOptParserImpl,
  GenArmNeon,
  GenArmNeonSema,
  GenArmNeonTest,
  GenArm64SIMD,
  GenArm64SIMDSema,
  GenArm64SIMDTest
};

namespace {
  cl::opt<ActionType>
  Action(cl::desc("Action to perform:"),
         cl::values(clEnumValN(GenOptParserDefs, "gen-opt-parser-defs",
                               "Generate option definitions"),
                    clEnumValN(GenOptParserImpl, "gen-opt-parser-impl",
                               "Generate option parser implementation"),
                    clEnumValN(GenClangAttrClasses, "gen-clang-attr-classes",
                               "Generate clang attribute clases"),
                    clEnumValN(GenClangAttrImpl, "gen-clang-attr-impl",
                               "Generate clang attribute implementations"),
                    clEnumValN(GenClangAttrList, "gen-clang-attr-list",
                               "Generate a clang attribute list"),
                    clEnumValN(GenClangAttrPCHRead, "gen-clang-attr-pch-read",
                               "Generate clang PCH attribute reader"),
                    clEnumValN(GenClangAttrPCHWrite, "gen-clang-attr-pch-write",
                               "Generate clang PCH attribute writer"),
                    clEnumValN(GenClangAttrSpellingList,
                               "gen-clang-attr-spelling-list",
                               "Generate a clang attribute spelling list"),
                    clEnumValN(GenClangAttrLateParsedList,
                               "gen-clang-attr-late-parsed-list",
                               "Generate a clang attribute LateParsed list"),
                    clEnumValN(GenClangAttrTemplateInstantiate,
                               "gen-clang-attr-template-instantiate",
                               "Generate a clang template instantiate code"),
                    clEnumValN(GenClangAttrParsedAttrList,
                               "gen-clang-attr-parsed-attr-list",
                               "Generate a clang parsed attribute list"),
                    clEnumValN(GenClangAttrParsedAttrKinds,
                               "gen-clang-attr-parsed-attr-kinds",
                               "Generate a clang parsed attribute kinds"),
                    clEnumValN(GenClangDiagsDefs, "gen-clang-diags-defs",
                               "Generate Clang diagnostics definitions"),
                    clEnumValN(GenClangDiagGroups, "gen-clang-diag-groups",
                               "Generate Clang diagnostic groups"),
                    clEnumValN(GenClangDiagsIndexName,
                               "gen-clang-diags-index-name",
                               "Generate Clang diagnostic name index"),
                    clEnumValN(GenClangDeclNodes, "gen-clang-decl-nodes",
                               "Generate Clang AST declaration nodes"),
                    clEnumValN(GenClangStmtNodes, "gen-clang-stmt-nodes",
                               "Generate Clang AST statement nodes"),
                    clEnumValN(GenClangSACheckers, "gen-clang-sa-checkers",
                               "Generate Clang Static Analyzer checkers"),
                    clEnumValN(GenArmNeon, "gen-arm-neon",
                               "Generate arm_neon.h for clang"),
                    clEnumValN(GenArmNeonSema, "gen-arm-neon-sema",
                               "Generate ARM NEON sema support for clang"),
                    clEnumValN(GenArmNeonTest, "gen-arm-neon-test",
                               "Generate ARM NEON tests for clang"),
                    clEnumValN(GenArm64SIMD, "gen-arm64-simd",
                               "Generate aarch64_simd.h for clang"),
                    clEnumValN(GenArm64SIMDSema, "gen-arm64-simd-sema",
                               "Generate ARM64 SIMD sema support for clang"),
                    clEnumValN(GenArm64SIMDTest, "gen-arm64-simd-test",
                               "Generate ARM64 SIMD tests for clang"),
                    clEnumValEnd));

  cl::opt<std::string>
  ClangComponent("clang-component",
                 cl::desc("Only use warnings from specified component"),
                 cl::value_desc("component"), cl::Hidden);

class ClangTableGenAction : public TableGenAction {
public:
  bool operator()(raw_ostream &OS, RecordKeeper &Records) {
    switch (Action) {
    case GenClangAttrClasses:
      EmitClangAttrClass(Records, OS);
      break;
    case GenClangAttrImpl:
      EmitClangAttrImpl(Records, OS);
      break;
    case GenClangAttrList:
      EmitClangAttrList(Records, OS);
      break;
    case GenClangAttrPCHRead:
      EmitClangAttrPCHRead(Records, OS);
      break;
    case GenClangAttrPCHWrite:
      EmitClangAttrPCHWrite(Records, OS);
      break;
    case GenClangAttrSpellingList:
      EmitClangAttrSpellingList(Records, OS);
      break;
    case GenClangAttrLateParsedList:
      EmitClangAttrLateParsedList(Records, OS);
      break;
    case GenClangAttrTemplateInstantiate:
      EmitClangAttrTemplateInstantiate(Records, OS);
      break;
    case GenClangAttrParsedAttrList:
      EmitClangAttrParsedAttrList(Records, OS);
      break;
    case GenClangAttrParsedAttrKinds:
      EmitClangAttrParsedAttrKinds(Records, OS);
      break;
    case GenClangDiagsDefs:
      EmitClangDiagsDefs(Records, OS, ClangComponent);
      break;
    case GenClangDiagGroups:
      EmitClangDiagGroups(Records, OS);
      break;
    case GenClangDiagsIndexName:
      EmitClangDiagsIndexName(Records, OS);
      break;
    case GenClangDeclNodes:
      EmitClangASTNodes(Records, OS, "Decl", "Decl");
      EmitClangDeclContext(Records, OS);
      break;
    case GenClangStmtNodes:
      EmitClangASTNodes(Records, OS, "Stmt", "");
      break;
    case GenClangSACheckers:
      EmitClangSACheckers(Records, OS);
      break;
    case GenOptParserDefs:
      EmitOptParser(Records, OS, true);
      break;
    case GenOptParserImpl:
      EmitOptParser(Records, OS, false);
      break;
    case GenArmNeon:
      EmitNeon(Records, OS);
      break;
    case GenArmNeonSema:
      EmitNeonSema(Records, OS);
      break;
    case GenArmNeonTest:
      EmitNeonTest(Records, OS);
      break;
    case GenArm64SIMD:
      NeonEmitter(Records, true).run(OS);
      break;
    case GenArm64SIMDSema:
      NeonEmitter(Records, true).runHeader(OS);
      break;
    case GenArm64SIMDTest:
      NeonEmitter(Records, true).runTests(OS);
      break;
    }

    return false;
  }
};
}

int main(int argc, char **argv) {
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);
  cl::ParseCommandLineOptions(argc, argv);

  ClangTableGenAction Action;
  return TableGenMain(argv[0], Action);
}
