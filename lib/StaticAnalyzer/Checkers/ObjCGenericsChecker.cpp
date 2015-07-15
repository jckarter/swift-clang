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
#include "clang/AST/ParentMap.h"
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
    : public Checker<check::DeadSymbols, check::PreObjCMessage,
                     check::PostStmt<CastExpr>> {
public:
  ProgramStateRef checkPointerEscape(ProgramStateRef State,
                                     const InvalidatedSymbols &Escaped,
                                     const CallEvent *Call,
                                     PointerEscapeKind Kind) const;

  void checkPreObjCMessage(const ObjCMethodCall &M, CheckerContext &C) const;
  void checkPostStmt(const CastExpr *CE, CheckerContext &C) const;
  void checkDeadSymbols(SymbolReaper &SR, CheckerContext &C) const;

private:
  mutable std::unique_ptr<BugType> BT;
  void initBugType() const {
    if (!BT)
      BT.reset(
          new BugType(this, "Generics", categories::CoreFoundationObjectiveC));
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
    SmallString<64> Buf;
    llvm::raw_svector_ostream OS(Buf);
    OS << "Incompatible pointer types assigning to '";
    QualType::print(From, Qualifiers(), OS, C.getLangOpts(), llvm::Twine());
    OS << "' from '";
    QualType::print(To, Qualifiers(), OS, C.getLangOpts(), llvm::Twine());
    OS << "'";
    std::unique_ptr<BugReport> R(new BugReport(*BT, OS.str(), N));
    R->markInteresting(Sym);
    R->addVisitor(llvm::make_unique<GenericsBugVisitor>(Sym));
    C.emitReport(std::move(R));
  }
};
} // end anonymous namespace

// ProgramState trait - a map from symbol to its type with specified params.
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

  // Retrieve the associated statement.
  const Stmt *S = nullptr;
  ProgramPoint ProgLoc = N->getLocation();
  if (Optional<StmtPoint> SP = ProgLoc.getAs<StmtPoint>()) {
    S = SP->getStmt();
  }

  if (!S)
    return nullptr;

  std::string TypeName = QualType::getAsString(*TrackedType, Qualifiers());
  std::string InfoText =
      (llvm::Twine("Type '") + TypeName + "' is infered from this context'")
          .str();

  // Generate the extra diagnostic.
  PathDiagnosticLocation Pos(S, BRC.getSourceManager(),
                             N->getLocationContext());
  return new PathDiagnosticEventPiece(Pos, InfoText, true, nullptr);
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
  const auto *SuperOfTo =
      To->getObjectType()->getSuperClassType()->getAs<ObjCObjectType>();
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

  ASTContext &ASTCtxt = C.getASTContext();

  if (!OrigObjectPtrType || !DestObjectPtrType)
    return;

  // In order to detect subtype relation properly, strip the kindofness.
  OrigObjectPtrType = OrigObjectPtrType->stripObjCKindOfTypeAndQuals(ASTCtxt);
  DestObjectPtrType = DestObjectPtrType->stripObjCKindOfTypeAndQuals(ASTCtxt);

  const ObjCObjectType *OrigObjectType = OrigObjectPtrType->getObjectType();
  const ObjCObjectType *DestObjectType = DestObjectPtrType->getObjectType();

  if (OrigObjectType->isUnspecialized() && DestObjectType->isUnspecialized())
    return;

  ProgramStateRef State = C.getState();
  SymbolRef Sym = State->getSVal(CE, C.getLocationContext()).getAsSymbol();
  if (!Sym)
    return;

  // Check which assignments are legal.
  bool OrigToDest =
      ASTCtxt.canAssignObjCInterfaces(DestObjectPtrType, OrigObjectPtrType);
  bool DestToOrig =
      ASTCtxt.canAssignObjCInterfaces(OrigObjectPtrType, DestObjectPtrType);
  const ObjCObjectPointerType *const *TrackedType =
      State->get<TypeParamMap>(Sym);

  // If OrigObjectType could convert to DestObjectType, this could be an
  // implicit cast. Handle it as implicit cast.
  if (isa<ExplicitCastExpr>(CE) && !OrigToDest) {
    // Trust explicit downcasts.
    // However a downcast may also lose information. E. g.:
    //   MutableMap<T, U> : Map
    // The downcast to mutable map loses the information about the types of the
    // map, and in general there is no way to recover that information from the
    // declaration. So no checks possible against APIs that expect specialized
    // Maps.
    if (DestToOrig) {
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
    // When upcast happens, store the type with the most information about the
    // type parameters.
    if (OrigToDest && !DestToOrig) {
      const ObjCObjectPointerType *WithMostInfo =
          getMostInformativeDerivedClass(DestObjectPtrType, OrigObjectPtrType,
                                         C.getASTContext());
      // When an (implicit) upcast or a downcast happens according to static
      // types,the destination type of the cast may contradict the tracked type.
      // In this case a warning should be emitted.
      if (TrackedType &&
          !ASTCtxt.canAssignObjCInterfaces(DestObjectPtrType, *TrackedType) &&
          !ASTCtxt.canAssignObjCInterfaces(*TrackedType, DestObjectPtrType)) {
        ExplodedNode *N = C.addTransition();
        reportBug(*TrackedType, DestObjectPtrType, N, Sym, C);
        return;
      }
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
      // Just found out what the type of this symbol should be.
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

void ObjCGenericsChecker::checkPreObjCMessage(const ObjCMethodCall &M,
                                               CheckerContext &C) const {
  const ObjCMessageExpr *MessageExpr = M.getOriginExpr();
  if (MessageExpr->getReceiverKind() != ObjCMessageExpr::Instance)
    return;

  ProgramStateRef State = C.getState();
  SymbolRef Sym = M.getReceiverSVal().getAsSymbol();
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

  ASTContext &ASTCtxt = C.getASTContext();

  // Get the type arguments from tracked type and substitute type arguments
  // before do the semantic check.

  // When the receiver type is id, or some super class of the tracked type (and
  // kindof type), look up the method in the tracked type, not in the receiver
  // type. This way we preserve more information.
  Selector Sel = MessageExpr->getSelector();
  const ObjCMethodDecl *Method = nullptr;
  if (ASTCtxt.getObjCIdType() == ReceiverType ||
      (ReceiverObjectPtrType->getObjectType()->isKindOfType() &&
       ASTCtxt.canAssignObjCInterfaces(ReceiverObjectPtrType, *TrackedType))) {
    const ObjCInterfaceDecl *InterfaceDecl = (*TrackedType)->getInterfaceDecl();
    // The method might not be found.
    Method = InterfaceDecl->lookupInstanceMethod(Sel);
  }

  if (!Method) {
    Method = MessageExpr->getMethodDecl();
    // When arc is disabled non-existent methods can be called.
    if (!Method)
      return;
  }

  Optional<ArrayRef<QualType>> TypeArgs =
      (*TrackedType)->getObjCSubstitutions(Method->getDeclContext());
  // This case might happen when there is an unspecialized override of a
  // specialized method.
  if (!TypeArgs)
    return;

  for (unsigned i = 0; i < Method->param_size(); i++) {
    const Expr *Arg = MessageExpr->getArg(i);
    // We can't do any type-checking on a type-dependent argument.
    if (Arg->isTypeDependent())
      continue;

    const ParmVarDecl *Param = Method->parameters()[i];

    QualType OrigParamType = Param->getType();
    const auto *ParamTypedef = OrigParamType->getAs<TypedefType>();
    if (!ParamTypedef)
      continue;

    const auto *TypeParamDecl =
        dyn_cast<ObjCTypeParamDecl>(ParamTypedef->getDecl());
    if (!TypeParamDecl)
      continue;

    ObjCTypeParamVariance ParamVariance = TypeParamDecl->getVariance();

    QualType ParamType = OrigParamType.substObjCTypeArgs(
        ASTCtxt, *TypeArgs, ObjCSubstitutionContext::Parameter);
    // Check if it can be assigned
    const auto *ParamObjectPtrType = ParamType->getAs<ObjCObjectPointerType>();
    const auto *ArgObjectPtrType = stripImplicitIdCast(Arg, ASTCtxt)
                                       ->getType()
                                       ->getAs<ObjCObjectPointerType>();
    if (!ParamObjectPtrType || !ArgObjectPtrType)
      continue;

    // For covariant type parameters every subclasses and supertypes are both
    // accepted.
    // FIXME: in case the argument is actually tracked, use the tracked type.
    if (!ASTCtxt.canAssignObjCInterfaces(ParamObjectPtrType,
                                         ArgObjectPtrType) &&
        (ParamVariance != ObjCTypeParamVariance::Covariant ||
         !ASTCtxt.canAssignObjCInterfaces(ArgObjectPtrType,
                                          ParamObjectPtrType))) {
      ExplodedNode *N = C.addTransition();
      reportBug(ArgObjectPtrType, ParamObjectPtrType, N, Sym, C);
      return;
    }
  }
  QualType ResultType = Method->getReturnType().substObjCTypeArgs(
      ASTCtxt, *TypeArgs, ObjCSubstitutionContext::Result);
  QualType StaticResultType = Method->getReturnType();
  // Check whether the result type was a type parameter.
  const auto *ResultTypedef = StaticResultType->getAs<TypedefType>();
  if (!ResultTypedef)
    return;

  if (!isa<ObjCTypeParamDecl>(ResultTypedef->getDecl()))
    return;

  const Stmt *Parent =
      C.getCurrentAnalysisDeclContext()->getParentMap().getParent(MessageExpr);

  // FIXME: when accessing properties the cast might not be the direct parent.
  const auto *ImplicitCast = dyn_cast_or_null<ImplicitCastExpr>(Parent);
  if (!ImplicitCast || ImplicitCast->getCastKind() != CK_BitCast)
    return;

  const auto *ExprTypeAboveCast =
      ImplicitCast->getType()->getAs<ObjCObjectPointerType>();
  const auto *ResultPtrType = ResultType->getAs<ObjCObjectPointerType>();

  if (!ExprTypeAboveCast || !ResultPtrType)
    return;

  // Only warn on unrelated types to avoid too many false positives on
  // downcasts.
  if (!ASTCtxt.canAssignObjCInterfaces(ExprTypeAboveCast, ResultPtrType) &&
      !ASTCtxt.canAssignObjCInterfaces(ResultPtrType, ExprTypeAboveCast)) {
    ExplodedNode *N = C.addTransition();
    reportBug(ResultPtrType, ExprTypeAboveCast, N, Sym, C);
    return;
  }
}

/// Register checker.
void ento::registerObjCGenericsChecker(CheckerManager &mgr) {
  mgr.registerChecker<ObjCGenericsChecker>();
}
