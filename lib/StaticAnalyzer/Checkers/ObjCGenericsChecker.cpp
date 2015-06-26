//=== ObjCGenericsChecker.cpp - Path sensitive checker for Generics *- C++ -*=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// TODO: Description.
//
//===----------------------------------------------------------------------===//

#include "ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"
using namespace clang;
using namespace ento;

namespace {
class ObjCGenericsChecker
    : public Checker<check::DeadSymbols, check::PostObjCMessage,
                     check::PostStmt<CastExpr>> {
public:
  /// A tag to id this checker.
  static void *getTag() {
    static int Tag;
    return &Tag;
  }

  ProgramStateRef checkPointerEscape(ProgramStateRef State,
                                     const InvalidatedSymbols &Escaped,
                                     const CallEvent *Call,
                                     PointerEscapeKind Kind) const;

  void checkPostObjCMessage(const ObjCMethodCall &M, CheckerContext &C) const;
  void checkPostStmt(const CastExpr *CE, CheckerContext &C) const;
  void checkDeadSymbols(SymbolReaper &SR, CheckerContext &C) const;

private:
  mutable std::unique_ptr<BugType> BT;
  void initBugType() const {
    if (!BT)
      BT.reset(
          new BugType(this, "Generics.", categories::CoreFoundationObjectiveC));
  }

  class GenericsBugVisitor : public BugReporterVisitorImpl<GenericsBugVisitor> {
  public:
    GenericsBugVisitor(SymbolRef S) : Sym(S) {}
    ~GenericsBugVisitor() override {}

    void Profile(llvm::FoldingSetNodeID &ID) const override {
      static int X = 0;
      ID.AddPointer(&X);
      ID.AddPointer(Sym);
    }

    PathDiagnosticPiece *VisitNode(const ExplodedNode *N,
                                   const ExplodedNode *PrevN,
                                   BugReporterContext &BRC,
                                   BugReport &BR) override;

  private:
    // The tracked symbol.
    SymbolRef Sym;
  };

  void reportBug(const ObjCObjectPointerType *From,
                 const ObjCObjectPointerType *To, ExplodedNode *N,
                 SymbolRef Sym, CheckerContext &C) const {
    initBugType();
    std::string FromStr = QualType::getAsString(From, Qualifiers());
    std::string ToStr = QualType::getAsString(To, Qualifiers());
    std::string ErrorText =
        (llvm::Twine("incompatible pointer types assigning to '") + ToStr +
         "' from '" + FromStr + "'")
            .str();
    std::unique_ptr<BugReport> R(new BugReport(*BT, ErrorText, N));
    R->markInteresting(Sym);
    R->addVisitor(llvm::make_unique<GenericsBugVisitor>(Sym));
    C.emitReport(std::move(R));
  }
};
} // end anonymous namespace

// ProgramState trait - a map from symbol to its typewith specified params.
REGISTER_MAP_WITH_PROGRAMSTATE(TypeParamMap, SymbolRef,
                               const ObjCObjectPointerType *)

PathDiagnosticPiece *ObjCGenericsChecker::GenericsBugVisitor::VisitNode(
    const ExplodedNode *N, const ExplodedNode *PrevN, BugReporterContext &BRC,
    BugReport &BR) {
  ProgramStateRef state = N->getState();
  ProgramStateRef statePrev = PrevN->getState();

  const ObjCObjectPointerType *const *TrackedType =
      state->get<TypeParamMap>(Sym);
  const ObjCObjectPointerType *const *TrackedTypePrev =
      statePrev->get<TypeParamMap>(Sym);
  if (!TrackedType)
    return nullptr;

  if (TrackedTypePrev && *TrackedTypePrev == *TrackedType)
    return nullptr;

  const Stmt *S = nullptr;
  StackHintGeneratorForSymbol *StackHint = nullptr;

  // Retrieve the associated statement.
  ProgramPoint ProgLoc = N->getLocation();
  if (Optional<StmtPoint> SP = ProgLoc.getAs<StmtPoint>()) {
    S = SP->getStmt();
  }

  if (!S)
    return nullptr;

  // Generate the extra diagnostic.
  PathDiagnosticLocation Pos(S, BRC.getSourceManager(),
                             N->getLocationContext());
  return new PathDiagnosticEventPiece(
      Pos, "Type information inferred from this location.", true, StackHint);
}

void ObjCGenericsChecker::checkDeadSymbols(SymbolReaper &SR,
                                           CheckerContext &C) const {
  if (!SR.hasDeadSymbols())
    return;

  ProgramStateRef State = C.getState();
  TypeParamMapTy TyParMap = State->get<TypeParamMap>();
  for (TypeParamMapTy::iterator I = TyParMap.begin(), E = TyParMap.end();
       I != E; ++I) {
    if (SR.isDead(I->first)) {
      State = State->remove<TypeParamMap>(I->first);
    }
  }
}

static const ObjCObjectPointerType *getMostInformativeDerivedClassImpl(
    const ObjCObjectPointerType *From, const ObjCObjectPointerType *To,
    const ObjCObjectPointerType *MostInformativeCandidate, ASTContext &C) {
  // Checking if from and two are the same classes modulo specialization.
  if (From->getInterfaceDecl()->getCanonicalDecl() ==
      To->getInterfaceDecl()->getCanonicalDecl()) {
    if (To->isSpecialized()) {
      assert(MostInformativeCandidate->isSpecialized());
      return MostInformativeCandidate;
    }
    return From;
  }
  const ObjCInterfaceDecl *SuperOfToDecl =
      To->getObjectType()->getInterface()->getSuperClass();
  assert(SuperOfToDecl);
  const auto *SuperOfTo =
      SuperOfToDecl->getTypeForDecl()->getAs<ObjCObjectType>();
  assert(SuperOfTo);
  QualType SuperPtrOfToQual =
      C.getObjCObjectPointerType(QualType(SuperOfTo, 0));
  const auto *SuperPtrOfTo = SuperPtrOfToQual->getAs<ObjCObjectPointerType>();
  if (To->isUnspecialized())
    return getMostInformativeDerivedClassImpl(From, SuperPtrOfTo, SuperPtrOfTo,
                                              C);
  else
    return getMostInformativeDerivedClassImpl(From, SuperPtrOfTo,
                                              MostInformativeCandidate, C);
}

static const ObjCObjectPointerType *
getMostInformativeDerivedClass(const ObjCObjectPointerType *From,
                               const ObjCObjectPointerType *To, ASTContext &C) {
  return getMostInformativeDerivedClassImpl(From, To, To, C);
}

static bool storeWhenMoreInformative(ProgramStateRef &State, SymbolRef Sym,
                                     const ObjCObjectPointerType *const *Old,
                                     const ObjCObjectPointerType *New,
                                     ASTContext &C) {
  if (!Old || C.canAssignObjCInterfaces(*Old, New)) {
    State = State->set<TypeParamMap>(Sym, New);
    return true;
  }
  return false;
}

void ObjCGenericsChecker::checkPostStmt(const CastExpr *CE,
                                        CheckerContext &C) const {
  if (CE->getCastKind() != CK_BitCast)
    return;

  QualType OriginType = CE->getSubExpr()->getType();
  QualType DestType = CE->getType();

  const auto *OrigObjectPtrType = OriginType->getAs<ObjCObjectPointerType>();
  const auto *DestObjectPtrType = DestType->getAs<ObjCObjectPointerType>();

  if (!OrigObjectPtrType || !DestObjectPtrType)
    return;

  const ObjCObjectType *OrigObjectType = OrigObjectPtrType->getObjectType();
  const ObjCObjectType *DestObjectType = DestObjectPtrType->getObjectType();

  if (OrigObjectType->isUnspecialized() && DestObjectType->isUnspecialized())
    return;

  ProgramStateRef State = C.getState();
  SymbolRef Sym = State->getSVal(CE, C.getLocationContext()).getAsSymbol();
  if (!Sym)
    return;

  ASTContext &ASTCtxt = C.getASTContext();
  // Check which assignments are legal.
  bool OrigToDest =
      ASTCtxt.canAssignObjCInterfaces(DestObjectPtrType, OrigObjectPtrType);
  bool DestToOrig =
      ASTCtxt.canAssignObjCInterfaces(OrigObjectPtrType, DestObjectPtrType);
  const ObjCObjectPointerType *const *TrackedType =
      State->get<TypeParamMap>(Sym);

  if (isa<ExplicitCastExpr>(CE)) {
    // Trust explicit downcasts.
    // However a downcast may also lose information. E. g.:
    //   MutableMap<T, U> : Map
    // The downcast to mutable map loses the information about the types of the
    // map, and in general there is no way to recover that information from the
    // declaration. So no checks possible against APIs that expect specialized
    // Maps.
    if (DestToOrig && !OrigToDest) {
      const ObjCObjectPointerType *WithMostInfo =
          getMostInformativeDerivedClass(OrigObjectPtrType, DestObjectPtrType,
                                         C.getASTContext());
      if (storeWhenMoreInformative(State, Sym, TrackedType, WithMostInfo,
                                   ASTCtxt))
        C.addTransition(State);
    }
    return;
  }

  if (DestObjectType->isUnspecialized()) {
    // In case we already have some type information for this symbol from a
    // Specialized -> Specialized conversion, do not record the OrigType,
    // because it might contain less type information than the tracked type.
    assert(OrigObjectType->isSpecialized());
    if (!TrackedType) {
      State = State->set<TypeParamMap>(Sym, OrigObjectPtrType);
      C.addTransition(State);
    }
  } else {
    // When upcast happens, store the type with the most information.
    if (OrigToDest && !DestToOrig) {
      const ObjCObjectPointerType *WithMostInfo =
          getMostInformativeDerivedClass(DestObjectPtrType, OrigObjectPtrType,
                                         C.getASTContext());
      if (storeWhenMoreInformative(State, Sym, TrackedType, WithMostInfo,
                                   ASTCtxt))
        C.addTransition(State);
      return;
    }
    // Trust tracked type on unspecialized value -> specialized implicit
    // downcasts.
    if (TrackedType) {
      if (storeWhenMoreInformative(State, Sym, TrackedType, DestObjectPtrType,
                                   ASTCtxt)) {
        C.addTransition(State);
        return;
      }
      // Illegal cast.
      if (!ASTCtxt.canAssignObjCInterfaces(DestObjectPtrType, *TrackedType)) {
        ExplodedNode *N = C.addTransition();
        reportBug(*TrackedType, DestObjectPtrType, N, Sym, C);
      }
    } else {
      // Just found out, what the type of this symbol should be.
      State = State->set<TypeParamMap>(Sym, DestObjectPtrType);
      C.addTransition(State);
    }
  }
}

static const Expr *stripImplicitIdCast(const Expr *E, ASTContext &ASTCtxt) {
  const ImplicitCastExpr *CE = dyn_cast<ImplicitCastExpr>(E);
  if (CE && CE->getCastKind() == CK_BitCast &&
      CE->getType() == ASTCtxt.getObjCIdType())
    return CE->getSubExpr();
  else
    return E;
}

void ObjCGenericsChecker::checkPostObjCMessage(const ObjCMethodCall &M,
                                               CheckerContext &C) const {
  const ObjCMessageExpr *MessageExpr = M.getOriginExpr();
  if (MessageExpr->getReceiverKind() != ObjCMessageExpr::Instance)
    return;

  const Expr *InstanceExpr = MessageExpr->getInstanceReceiver();

  ProgramStateRef State = C.getState();
  SymbolRef Sym =
      State->getSVal(InstanceExpr, C.getLocationContext()).getAsSymbol();
  if (!Sym)
    return;

  QualType ReceiverType = MessageExpr->getReceiverType();

  const auto *ReceiverObjectPtrType =
      ReceiverType->getAs<ObjCObjectPointerType>();

  if (!ReceiverObjectPtrType)
    return;

  const ObjCObjectPointerType *const *TrackedType =
      State->get<TypeParamMap>(Sym);
  if (!TrackedType)
    return;

  // Get the type arguments from tracked type and substitute type arguments
  // before do the semantic check.

  const ObjCMethodDecl *Method = MessageExpr->getMethodDecl();
  Selector Sel = MessageExpr->getSelector();

  unsigned NumNamedArgs = Sel.getNumArgs();
  // Method might have more arguments than selector indicates. This is due
  // to addition of c-style arguments in method.
  if (Method->param_size() > Sel.getNumArgs())
    NumNamedArgs = Method->param_size();

  Optional<ArrayRef<QualType>> typeArgs =
      (*TrackedType)->getObjCSubstitutions(Method->getDeclContext());
  // This case might happen, when there is an unspecialized override of a
  // specialized method.
  if (!typeArgs)
    return;

  ASTContext &ASTCtxt = C.getASTContext();
  for (unsigned i = 0; i < NumNamedArgs; i++) {
    const Expr *Arg = MessageExpr->getArg(i);
    // We can't do any type-checking on a type-dependent argument.
    if (Arg->isTypeDependent())
      continue;

    const ParmVarDecl *Param = Method->parameters()[i];

    QualType OrigParamType = Param->getType();
    QualType ParamType = OrigParamType.substObjCTypeArgs(
        ASTCtxt, *typeArgs, ObjCSubstitutionContext::Parameter);
    // Check if it can be assigned
    const auto *ParamObjectPtrType = ParamType->getAs<ObjCObjectPointerType>();
    const auto *ArgObjectPtrType = stripImplicitIdCast(Arg, ASTCtxt)
                                       ->getType()
                                       ->getAs<ObjCObjectPointerType>();
    if (!ParamObjectPtrType || !ArgObjectPtrType)
      continue;

    if (!ASTCtxt.canAssignObjCInterfaces(ParamObjectPtrType,
                                         ArgObjectPtrType)) {
      ExplodedNode *N = C.addTransition();
      reportBug(ArgObjectPtrType, ParamObjectPtrType, N, Sym, C);
      return;
    }
  }
}

/// Register checker.
void ento::registerObjCGenericsChecker(CheckerManager &mgr) {
  mgr.registerChecker<ObjCGenericsChecker>();
}
