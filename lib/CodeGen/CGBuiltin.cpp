//===---- CGBuiltin.cpp - Emit LLVM Code for builtins ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This contains code to emit Builtin calls as LLVM code.
//
//===----------------------------------------------------------------------===//

#include "CodeGenFunction.h"
#include "CGObjCRuntime.h"
#include "CodeGenModule.h"
#include "TargetInfo.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/Basic/TargetBuiltins.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/CodeGen/CGFunctionInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Intrinsics.h"

using namespace clang;
using namespace CodeGen;
using namespace llvm;

/// getBuiltinLibFunction - Given a builtin id for a function like
/// "__builtin_fabsf", return a Function* for "fabsf".
llvm::Value *CodeGenModule::getBuiltinLibFunction(const FunctionDecl *FD,
                                                  unsigned BuiltinID) {
  assert(Context.BuiltinInfo.isLibFunction(BuiltinID));

  // Get the name, skip over the __builtin_ prefix (if necessary).
  StringRef Name;
  GlobalDecl D(FD);

  // If the builtin has been declared explicitly with an assembler label,
  // use the mangled name. This differs from the plain label on platforms
  // that prefix labels.
  if (FD->hasAttr<AsmLabelAttr>())
    Name = getMangledName(D);
  else
    Name = Context.BuiltinInfo.GetName(BuiltinID) + 10;

  llvm::FunctionType *Ty =
    cast<llvm::FunctionType>(getTypes().ConvertType(FD->getType()));

  return GetOrCreateLLVMFunction(Name, Ty, D, /*ForVTable=*/false);
}

/// Emit the conversions required to turn the given value into an
/// integer of the given size.
static Value *EmitToInt(CodeGenFunction &CGF, llvm::Value *V,
                        QualType T, llvm::IntegerType *IntType) {
  V = CGF.EmitToMemory(V, T);

  if (V->getType()->isPointerTy())
    return CGF.Builder.CreatePtrToInt(V, IntType);

  assert(V->getType() == IntType);
  return V;
}

static Value *EmitFromInt(CodeGenFunction &CGF, llvm::Value *V,
                          QualType T, llvm::Type *ResultType) {
  V = CGF.EmitFromMemory(V, T);

  if (ResultType->isPointerTy())
    return CGF.Builder.CreateIntToPtr(V, ResultType);

  assert(V->getType() == ResultType);
  return V;
}

/// Utility to insert an atomic instruction based on Instrinsic::ID
/// and the expression node.
static RValue EmitBinaryAtomic(CodeGenFunction &CGF,
                               llvm::AtomicRMWInst::BinOp Kind,
                               const CallExpr *E) {
  QualType T = E->getType();
  assert(E->getArg(0)->getType()->isPointerType());
  assert(CGF.getContext().hasSameUnqualifiedType(T,
                                  E->getArg(0)->getType()->getPointeeType()));
  assert(CGF.getContext().hasSameUnqualifiedType(T, E->getArg(1)->getType()));

  llvm::Value *DestPtr = CGF.EmitScalarExpr(E->getArg(0));
  unsigned AddrSpace = DestPtr->getType()->getPointerAddressSpace();

  llvm::IntegerType *IntType =
    llvm::IntegerType::get(CGF.getLLVMContext(),
                           CGF.getContext().getTypeSize(T));
  llvm::Type *IntPtrType = IntType->getPointerTo(AddrSpace);

  llvm::Value *Args[2];
  Args[0] = CGF.Builder.CreateBitCast(DestPtr, IntPtrType);
  Args[1] = CGF.EmitScalarExpr(E->getArg(1));
  llvm::Type *ValueType = Args[1]->getType();
  Args[1] = EmitToInt(CGF, Args[1], T, IntType);

  llvm::Value *Result =
      CGF.Builder.CreateAtomicRMW(Kind, Args[0], Args[1],
                                  llvm::SequentiallyConsistent);
  Result = EmitFromInt(CGF, Result, T, ValueType);
  return RValue::get(Result);
}

/// Utility to insert an atomic instruction based Instrinsic::ID and
/// the expression node, where the return value is the result of the
/// operation.
static RValue EmitBinaryAtomicPost(CodeGenFunction &CGF,
                                   llvm::AtomicRMWInst::BinOp Kind,
                                   const CallExpr *E,
                                   Instruction::BinaryOps Op) {
  QualType T = E->getType();
  assert(E->getArg(0)->getType()->isPointerType());
  assert(CGF.getContext().hasSameUnqualifiedType(T,
                                  E->getArg(0)->getType()->getPointeeType()));
  assert(CGF.getContext().hasSameUnqualifiedType(T, E->getArg(1)->getType()));

  llvm::Value *DestPtr = CGF.EmitScalarExpr(E->getArg(0));
  unsigned AddrSpace = DestPtr->getType()->getPointerAddressSpace();

  llvm::IntegerType *IntType =
    llvm::IntegerType::get(CGF.getLLVMContext(),
                           CGF.getContext().getTypeSize(T));
  llvm::Type *IntPtrType = IntType->getPointerTo(AddrSpace);

  llvm::Value *Args[2];
  Args[1] = CGF.EmitScalarExpr(E->getArg(1));
  llvm::Type *ValueType = Args[1]->getType();
  Args[1] = EmitToInt(CGF, Args[1], T, IntType);
  Args[0] = CGF.Builder.CreateBitCast(DestPtr, IntPtrType);

  llvm::Value *Result =
      CGF.Builder.CreateAtomicRMW(Kind, Args[0], Args[1],
                                  llvm::SequentiallyConsistent);
  Result = CGF.Builder.CreateBinOp(Op, Result, Args[1]);
  Result = EmitFromInt(CGF, Result, T, ValueType);
  return RValue::get(Result);
}

/// EmitFAbs - Emit a call to fabs/fabsf/fabsl, depending on the type of ValTy,
/// which must be a scalar floating point type.
static Value *EmitFAbs(CodeGenFunction &CGF, Value *V, QualType ValTy) {
  const BuiltinType *ValTyP = ValTy->getAs<BuiltinType>();
  assert(ValTyP && "isn't scalar fp type!");

  StringRef FnName;
  switch (ValTyP->getKind()) {
  default: llvm_unreachable("Isn't a scalar fp type!");
  case BuiltinType::Float:      FnName = "fabsf"; break;
  case BuiltinType::Double:     FnName = "fabs"; break;
  case BuiltinType::LongDouble: FnName = "fabsl"; break;
  }

  // The prototype is something that takes and returns whatever V's type is.
  llvm::FunctionType *FT = llvm::FunctionType::get(V->getType(), V->getType(),
                                                   false);
  llvm::Value *Fn = CGF.CGM.CreateRuntimeFunction(FT, FnName);

  return CGF.EmitNounwindRuntimeCall(Fn, V, "abs");
}

static RValue emitLibraryCall(CodeGenFunction &CGF, const FunctionDecl *Fn,
                              const CallExpr *E, llvm::Value *calleeValue) {
  return CGF.EmitCall(E->getCallee()->getType(), calleeValue, E->getLocStart(),
                      ReturnValueSlot(), E->arg_begin(), E->arg_end(), Fn);
}

/// \brief Emit a call to llvm.{sadd,uadd,ssub,usub,smul,umul}.with.overflow.*
/// depending on IntrinsicID.
///
/// \arg CGF The current codegen function.
/// \arg IntrinsicID The ID for the Intrinsic we wish to generate.
/// \arg X The first argument to the llvm.*.with.overflow.*.
/// \arg Y The second argument to the llvm.*.with.overflow.*.
/// \arg Carry The carry returned by the llvm.*.with.overflow.*.
/// \returns The result (i.e. sum/product) returned by the intrinsic.
static llvm::Value *EmitOverflowIntrinsic(CodeGenFunction &CGF,
                                          const llvm::Intrinsic::ID IntrinsicID,
                                          llvm::Value *X, llvm::Value *Y,
                                          llvm::Value *&Carry) {
  // Make sure we have integers of the same width.
  assert(X->getType() == Y->getType() &&
         "Arguments must be the same type. (Did you forget to make sure both "
         "arguments have the same integer width?)");

  llvm::Value *Callee = CGF.CGM.getIntrinsic(IntrinsicID, X->getType());
  llvm::Value *Tmp = CGF.Builder.CreateCall2(Callee, X, Y);
  Carry = CGF.Builder.CreateExtractValue(Tmp, 1);
  return CGF.Builder.CreateExtractValue(Tmp, 0);
}

RValue CodeGenFunction::EmitBuiltinExpr(const FunctionDecl *FD,
                                        unsigned BuiltinID, const CallExpr *E) {
  // See if we can constant fold this builtin.  If so, don't emit it at all.
  Expr::EvalResult Result;
  if (E->EvaluateAsRValue(Result, CGM.getContext()) &&
      !Result.hasSideEffects()) {
    if (Result.Val.isInt())
      return RValue::get(llvm::ConstantInt::get(getLLVMContext(),
                                                Result.Val.getInt()));
    if (Result.Val.isFloat())
      return RValue::get(llvm::ConstantFP::get(getLLVMContext(),
                                               Result.Val.getFloat()));
  }

  switch (BuiltinID) {
  default: break;  // Handle intrinsics and libm functions below.
  case Builtin::BI__builtin___CFStringMakeConstantString:
  case Builtin::BI__builtin___NSStringMakeConstantString:
    return RValue::get(CGM.EmitConstantExpr(E, E->getType(), 0));
  case Builtin::BI__builtin_stdarg_start:
  case Builtin::BI__builtin_va_start:
  case Builtin::BI__builtin_va_end: {
    Value *ArgValue = EmitVAListRef(E->getArg(0));
    llvm::Type *DestType = Int8PtrTy;
    if (ArgValue->getType() != DestType)
      ArgValue = Builder.CreateBitCast(ArgValue, DestType,
                                       ArgValue->getName().data());

    Intrinsic::ID inst = (BuiltinID == Builtin::BI__builtin_va_end) ?
      Intrinsic::vaend : Intrinsic::vastart;
    return RValue::get(Builder.CreateCall(CGM.getIntrinsic(inst), ArgValue));
  }
  case Builtin::BI__builtin_va_copy: {
    Value *DstPtr = EmitVAListRef(E->getArg(0));
    Value *SrcPtr = EmitVAListRef(E->getArg(1));

    llvm::Type *Type = Int8PtrTy;

    DstPtr = Builder.CreateBitCast(DstPtr, Type);
    SrcPtr = Builder.CreateBitCast(SrcPtr, Type);
    return RValue::get(Builder.CreateCall2(CGM.getIntrinsic(Intrinsic::vacopy),
                                           DstPtr, SrcPtr));
  }
  case Builtin::BI__builtin_abs:
  case Builtin::BI__builtin_labs:
  case Builtin::BI__builtin_llabs: {
    Value *ArgValue = EmitScalarExpr(E->getArg(0));

    Value *NegOp = Builder.CreateNeg(ArgValue, "neg");
    Value *CmpResult =
    Builder.CreateICmpSGE(ArgValue,
                          llvm::Constant::getNullValue(ArgValue->getType()),
                                                            "abscond");
    Value *Result =
      Builder.CreateSelect(CmpResult, ArgValue, NegOp, "abs");

    return RValue::get(Result);
  }

  case Builtin::BI__builtin_conj:
  case Builtin::BI__builtin_conjf:
  case Builtin::BI__builtin_conjl: {
    ComplexPairTy ComplexVal = EmitComplexExpr(E->getArg(0));
    Value *Real = ComplexVal.first;
    Value *Imag = ComplexVal.second;
    Value *Zero =
      Imag->getType()->isFPOrFPVectorTy()
        ? llvm::ConstantFP::getZeroValueForNegation(Imag->getType())
        : llvm::Constant::getNullValue(Imag->getType());

    Imag = Builder.CreateFSub(Zero, Imag, "sub");
    return RValue::getComplex(std::make_pair(Real, Imag));
  }
  case Builtin::BI__builtin_creal:
  case Builtin::BI__builtin_crealf:
  case Builtin::BI__builtin_creall:
  case Builtin::BIcreal:
  case Builtin::BIcrealf:
  case Builtin::BIcreall: {
    ComplexPairTy ComplexVal = EmitComplexExpr(E->getArg(0));
    return RValue::get(ComplexVal.first);
  }

  case Builtin::BI__builtin_cimag:
  case Builtin::BI__builtin_cimagf:
  case Builtin::BI__builtin_cimagl:
  case Builtin::BIcimag:
  case Builtin::BIcimagf:
  case Builtin::BIcimagl: {
    ComplexPairTy ComplexVal = EmitComplexExpr(E->getArg(0));
    return RValue::get(ComplexVal.second);
  }

  case Builtin::BI__builtin_ctzs:
  case Builtin::BI__builtin_ctz:
  case Builtin::BI__builtin_ctzl:
  case Builtin::BI__builtin_ctzll: {
    Value *ArgValue = EmitScalarExpr(E->getArg(0));

    llvm::Type *ArgType = ArgValue->getType();
    Value *F = CGM.getIntrinsic(Intrinsic::cttz, ArgType);

    llvm::Type *ResultType = ConvertType(E->getType());
    Value *ZeroUndef = Builder.getInt1(getTarget().isCLZForZeroUndef());
    Value *Result = Builder.CreateCall2(F, ArgValue, ZeroUndef);
    if (Result->getType() != ResultType)
      Result = Builder.CreateIntCast(Result, ResultType, /*isSigned*/true,
                                     "cast");
    return RValue::get(Result);
  }
  case Builtin::BI__builtin_clzs:
  case Builtin::BI__builtin_clz:
  case Builtin::BI__builtin_clzl:
  case Builtin::BI__builtin_clzll: {
    Value *ArgValue = EmitScalarExpr(E->getArg(0));

    llvm::Type *ArgType = ArgValue->getType();
    Value *F = CGM.getIntrinsic(Intrinsic::ctlz, ArgType);

    llvm::Type *ResultType = ConvertType(E->getType());
    Value *ZeroUndef = Builder.getInt1(getTarget().isCLZForZeroUndef());
    Value *Result = Builder.CreateCall2(F, ArgValue, ZeroUndef);
    if (Result->getType() != ResultType)
      Result = Builder.CreateIntCast(Result, ResultType, /*isSigned*/true,
                                     "cast");
    return RValue::get(Result);
  }
  case Builtin::BI__builtin_ffs:
  case Builtin::BI__builtin_ffsl:
  case Builtin::BI__builtin_ffsll: {
    // ffs(x) -> x ? cttz(x) + 1 : 0
    Value *ArgValue = EmitScalarExpr(E->getArg(0));

    llvm::Type *ArgType = ArgValue->getType();
    Value *F = CGM.getIntrinsic(Intrinsic::cttz, ArgType);

    llvm::Type *ResultType = ConvertType(E->getType());
    Value *Tmp = Builder.CreateAdd(Builder.CreateCall2(F, ArgValue,
                                                       Builder.getTrue()),
                                   llvm::ConstantInt::get(ArgType, 1));
    Value *Zero = llvm::Constant::getNullValue(ArgType);
    Value *IsZero = Builder.CreateICmpEQ(ArgValue, Zero, "iszero");
    Value *Result = Builder.CreateSelect(IsZero, Zero, Tmp, "ffs");
    if (Result->getType() != ResultType)
      Result = Builder.CreateIntCast(Result, ResultType, /*isSigned*/true,
                                     "cast");
    return RValue::get(Result);
  }
  case Builtin::BI__builtin_parity:
  case Builtin::BI__builtin_parityl:
  case Builtin::BI__builtin_parityll: {
    // parity(x) -> ctpop(x) & 1
    Value *ArgValue = EmitScalarExpr(E->getArg(0));

    llvm::Type *ArgType = ArgValue->getType();
    Value *F = CGM.getIntrinsic(Intrinsic::ctpop, ArgType);

    llvm::Type *ResultType = ConvertType(E->getType());
    Value *Tmp = Builder.CreateCall(F, ArgValue);
    Value *Result = Builder.CreateAnd(Tmp, llvm::ConstantInt::get(ArgType, 1));
    if (Result->getType() != ResultType)
      Result = Builder.CreateIntCast(Result, ResultType, /*isSigned*/true,
                                     "cast");
    return RValue::get(Result);
  }
  case Builtin::BI__builtin_popcount:
  case Builtin::BI__builtin_popcountl:
  case Builtin::BI__builtin_popcountll: {
    Value *ArgValue = EmitScalarExpr(E->getArg(0));

    llvm::Type *ArgType = ArgValue->getType();
    Value *F = CGM.getIntrinsic(Intrinsic::ctpop, ArgType);

    llvm::Type *ResultType = ConvertType(E->getType());
    Value *Result = Builder.CreateCall(F, ArgValue);
    if (Result->getType() != ResultType)
      Result = Builder.CreateIntCast(Result, ResultType, /*isSigned*/true,
                                     "cast");
    return RValue::get(Result);
  }
  case Builtin::BI__builtin_expect: {
    Value *ArgValue = EmitScalarExpr(E->getArg(0));
    llvm::Type *ArgType = ArgValue->getType();

    Value *FnExpect = CGM.getIntrinsic(Intrinsic::expect, ArgType);
    Value *ExpectedValue = EmitScalarExpr(E->getArg(1));

    Value *Result = Builder.CreateCall2(FnExpect, ArgValue, ExpectedValue,
                                        "expval");
    return RValue::get(Result);
  }
  case Builtin::BI__builtin_bswap16:
  case Builtin::BI__builtin_bswap32:
  case Builtin::BI__builtin_bswap64: {
    Value *ArgValue = EmitScalarExpr(E->getArg(0));
    llvm::Type *ArgType = ArgValue->getType();
    Value *F = CGM.getIntrinsic(Intrinsic::bswap, ArgType);
    return RValue::get(Builder.CreateCall(F, ArgValue));
  }
  case Builtin::BI__builtin_object_size: {
    // We rely on constant folding to deal with expressions with side effects.
    assert(!E->getArg(0)->HasSideEffects(getContext()) &&
           "should have been constant folded");

    // We pass this builtin onto the optimizer so that it can
    // figure out the object size in more complex cases.
    llvm::Type *ResType = ConvertType(E->getType());

    // LLVM only supports 0 and 2, make sure that we pass along that
    // as a boolean.
    Value *Ty = EmitScalarExpr(E->getArg(1));
    ConstantInt *CI = dyn_cast<ConstantInt>(Ty);
    assert(CI);
    uint64_t val = CI->getZExtValue();
    CI = ConstantInt::get(Builder.getInt1Ty(), (val & 0x2) >> 1);
    // FIXME: Get right address space.
    llvm::Type *Tys[] = { ResType, Builder.getInt8PtrTy(0) };
    Value *F = CGM.getIntrinsic(Intrinsic::objectsize, Tys);
    return RValue::get(Builder.CreateCall2(F, EmitScalarExpr(E->getArg(0)),CI));
  }
  case Builtin::BI__builtin_prefetch: {
    Value *Locality, *RW, *Address = EmitScalarExpr(E->getArg(0));
    // FIXME: Technically these constants should of type 'int', yes?
    RW = (E->getNumArgs() > 1) ? EmitScalarExpr(E->getArg(1)) :
      llvm::ConstantInt::get(Int32Ty, 0);
    Locality = (E->getNumArgs() > 2) ? EmitScalarExpr(E->getArg(2)) :
      llvm::ConstantInt::get(Int32Ty, 3);
    Value *Data = llvm::ConstantInt::get(Int32Ty, 1);
    Value *F = CGM.getIntrinsic(Intrinsic::prefetch);
    return RValue::get(Builder.CreateCall4(F, Address, RW, Locality, Data));
  }
  case Builtin::BI__builtin_readcyclecounter: {
    Value *F = CGM.getIntrinsic(Intrinsic::readcyclecounter);
    return RValue::get(Builder.CreateCall(F));
  }
  case Builtin::BI__builtin_trap: {
    Value *F = CGM.getIntrinsic(Intrinsic::trap);
    return RValue::get(Builder.CreateCall(F));
  }
  case Builtin::BI__debugbreak: {
    Value *F = CGM.getIntrinsic(Intrinsic::debugtrap);
    return RValue::get(Builder.CreateCall(F));
  }
  case Builtin::BI__builtin_unreachable: {
    if (SanOpts->Unreachable)
      EmitCheck(Builder.getFalse(), "builtin_unreachable",
                EmitCheckSourceLocation(E->getExprLoc()),
                ArrayRef<llvm::Value *>(), CRK_Unrecoverable);
    else
      Builder.CreateUnreachable();

    // We do need to preserve an insertion point.
    EmitBlock(createBasicBlock("unreachable.cont"));

    return RValue::get(0);
  }

  case Builtin::BI__builtin_powi:
  case Builtin::BI__builtin_powif:
  case Builtin::BI__builtin_powil: {
    Value *Base = EmitScalarExpr(E->getArg(0));
    Value *Exponent = EmitScalarExpr(E->getArg(1));
    llvm::Type *ArgType = Base->getType();
    Value *F = CGM.getIntrinsic(Intrinsic::powi, ArgType);
    return RValue::get(Builder.CreateCall2(F, Base, Exponent));
  }

  case Builtin::BI__builtin_isgreater:
  case Builtin::BI__builtin_isgreaterequal:
  case Builtin::BI__builtin_isless:
  case Builtin::BI__builtin_islessequal:
  case Builtin::BI__builtin_islessgreater:
  case Builtin::BI__builtin_isunordered: {
    // Ordered comparisons: we know the arguments to these are matching scalar
    // floating point values.
    Value *LHS = EmitScalarExpr(E->getArg(0));
    Value *RHS = EmitScalarExpr(E->getArg(1));

    switch (BuiltinID) {
    default: llvm_unreachable("Unknown ordered comparison");
    case Builtin::BI__builtin_isgreater:
      LHS = Builder.CreateFCmpOGT(LHS, RHS, "cmp");
      break;
    case Builtin::BI__builtin_isgreaterequal:
      LHS = Builder.CreateFCmpOGE(LHS, RHS, "cmp");
      break;
    case Builtin::BI__builtin_isless:
      LHS = Builder.CreateFCmpOLT(LHS, RHS, "cmp");
      break;
    case Builtin::BI__builtin_islessequal:
      LHS = Builder.CreateFCmpOLE(LHS, RHS, "cmp");
      break;
    case Builtin::BI__builtin_islessgreater:
      LHS = Builder.CreateFCmpONE(LHS, RHS, "cmp");
      break;
    case Builtin::BI__builtin_isunordered:
      LHS = Builder.CreateFCmpUNO(LHS, RHS, "cmp");
      break;
    }
    // ZExt bool to int type.
    return RValue::get(Builder.CreateZExt(LHS, ConvertType(E->getType())));
  }
  case Builtin::BI__builtin_isnan: {
    Value *V = EmitScalarExpr(E->getArg(0));
    V = Builder.CreateFCmpUNO(V, V, "cmp");
    return RValue::get(Builder.CreateZExt(V, ConvertType(E->getType())));
  }

  case Builtin::BI__builtin_isinf: {
    // isinf(x) --> fabs(x) == infinity
    Value *V = EmitScalarExpr(E->getArg(0));
    V = EmitFAbs(*this, V, E->getArg(0)->getType());

    V = Builder.CreateFCmpOEQ(V, ConstantFP::getInfinity(V->getType()),"isinf");
    return RValue::get(Builder.CreateZExt(V, ConvertType(E->getType())));
  }

  // TODO: BI__builtin_isinf_sign
  //   isinf_sign(x) -> isinf(x) ? (signbit(x) ? -1 : 1) : 0

  case Builtin::BI__builtin_isnormal: {
    // isnormal(x) --> x == x && fabsf(x) < infinity && fabsf(x) >= float_min
    Value *V = EmitScalarExpr(E->getArg(0));
    Value *Eq = Builder.CreateFCmpOEQ(V, V, "iseq");

    Value *Abs = EmitFAbs(*this, V, E->getArg(0)->getType());
    Value *IsLessThanInf =
      Builder.CreateFCmpULT(Abs, ConstantFP::getInfinity(V->getType()),"isinf");
    APFloat Smallest = APFloat::getSmallestNormalized(
                   getContext().getFloatTypeSemantics(E->getArg(0)->getType()));
    Value *IsNormal =
      Builder.CreateFCmpUGE(Abs, ConstantFP::get(V->getContext(), Smallest),
                            "isnormal");
    V = Builder.CreateAnd(Eq, IsLessThanInf, "and");
    V = Builder.CreateAnd(V, IsNormal, "and");
    return RValue::get(Builder.CreateZExt(V, ConvertType(E->getType())));
  }

  case Builtin::BI__builtin_isfinite: {
    // isfinite(x) --> x == x && fabs(x) != infinity;
    Value *V = EmitScalarExpr(E->getArg(0));
    Value *Eq = Builder.CreateFCmpOEQ(V, V, "iseq");

    Value *Abs = EmitFAbs(*this, V, E->getArg(0)->getType());
    Value *IsNotInf =
      Builder.CreateFCmpUNE(Abs, ConstantFP::getInfinity(V->getType()),"isinf");

    V = Builder.CreateAnd(Eq, IsNotInf, "and");
    return RValue::get(Builder.CreateZExt(V, ConvertType(E->getType())));
  }

  case Builtin::BI__builtin_fpclassify: {
    Value *V = EmitScalarExpr(E->getArg(5));
    llvm::Type *Ty = ConvertType(E->getArg(5)->getType());

    // Create Result
    BasicBlock *Begin = Builder.GetInsertBlock();
    BasicBlock *End = createBasicBlock("fpclassify_end", this->CurFn);
    Builder.SetInsertPoint(End);
    PHINode *Result =
      Builder.CreatePHI(ConvertType(E->getArg(0)->getType()), 4,
                        "fpclassify_result");

    // if (V==0) return FP_ZERO
    Builder.SetInsertPoint(Begin);
    Value *IsZero = Builder.CreateFCmpOEQ(V, Constant::getNullValue(Ty),
                                          "iszero");
    Value *ZeroLiteral = EmitScalarExpr(E->getArg(4));
    BasicBlock *NotZero = createBasicBlock("fpclassify_not_zero", this->CurFn);
    Builder.CreateCondBr(IsZero, End, NotZero);
    Result->addIncoming(ZeroLiteral, Begin);

    // if (V != V) return FP_NAN
    Builder.SetInsertPoint(NotZero);
    Value *IsNan = Builder.CreateFCmpUNO(V, V, "cmp");
    Value *NanLiteral = EmitScalarExpr(E->getArg(0));
    BasicBlock *NotNan = createBasicBlock("fpclassify_not_nan", this->CurFn);
    Builder.CreateCondBr(IsNan, End, NotNan);
    Result->addIncoming(NanLiteral, NotZero);

    // if (fabs(V) == infinity) return FP_INFINITY
    Builder.SetInsertPoint(NotNan);
    Value *VAbs = EmitFAbs(*this, V, E->getArg(5)->getType());
    Value *IsInf =
      Builder.CreateFCmpOEQ(VAbs, ConstantFP::getInfinity(V->getType()),
                            "isinf");
    Value *InfLiteral = EmitScalarExpr(E->getArg(1));
    BasicBlock *NotInf = createBasicBlock("fpclassify_not_inf", this->CurFn);
    Builder.CreateCondBr(IsInf, End, NotInf);
    Result->addIncoming(InfLiteral, NotNan);

    // if (fabs(V) >= MIN_NORMAL) return FP_NORMAL else FP_SUBNORMAL
    Builder.SetInsertPoint(NotInf);
    APFloat Smallest = APFloat::getSmallestNormalized(
        getContext().getFloatTypeSemantics(E->getArg(5)->getType()));
    Value *IsNormal =
      Builder.CreateFCmpUGE(VAbs, ConstantFP::get(V->getContext(), Smallest),
                            "isnormal");
    Value *NormalResult =
      Builder.CreateSelect(IsNormal, EmitScalarExpr(E->getArg(2)),
                           EmitScalarExpr(E->getArg(3)));
    Builder.CreateBr(End);
    Result->addIncoming(NormalResult, NotInf);

    // return Result
    Builder.SetInsertPoint(End);
    return RValue::get(Result);
  }

  case Builtin::BIalloca:
  case Builtin::BI_alloca:
  case Builtin::BI__builtin_alloca: {
    Value *Size = EmitScalarExpr(E->getArg(0));
    return RValue::get(Builder.CreateAlloca(Builder.getInt8Ty(), Size));
  }
  case Builtin::BIbzero:
  case Builtin::BI__builtin_bzero: {
    std::pair<llvm::Value*, unsigned> Dest =
        EmitPointerWithAlignment(E->getArg(0));
    Value *SizeVal = EmitScalarExpr(E->getArg(1));
    Builder.CreateMemSet(Dest.first, Builder.getInt8(0), SizeVal,
                         Dest.second, false);
    return RValue::get(Dest.first);
  }
  case Builtin::BImemcpy:
  case Builtin::BI__builtin_memcpy: {
    std::pair<llvm::Value*, unsigned> Dest =
        EmitPointerWithAlignment(E->getArg(0));
    std::pair<llvm::Value*, unsigned> Src =
        EmitPointerWithAlignment(E->getArg(1));
    Value *SizeVal = EmitScalarExpr(E->getArg(2));
    unsigned Align = std::min(Dest.second, Src.second);
    Builder.CreateMemCpy(Dest.first, Src.first, SizeVal, Align, false);
    return RValue::get(Dest.first);
  }

  case Builtin::BI__builtin___memcpy_chk: {
    // fold __builtin_memcpy_chk(x, y, cst1, cst2) to memcpy iff cst1<=cst2.
    llvm::APSInt Size, DstSize;
    if (!E->getArg(2)->EvaluateAsInt(Size, CGM.getContext()) ||
        !E->getArg(3)->EvaluateAsInt(DstSize, CGM.getContext()))
      break;
    if (Size.ugt(DstSize))
      break;
    std::pair<llvm::Value*, unsigned> Dest =
        EmitPointerWithAlignment(E->getArg(0));
    std::pair<llvm::Value*, unsigned> Src =
        EmitPointerWithAlignment(E->getArg(1));
    Value *SizeVal = llvm::ConstantInt::get(Builder.getContext(), Size);
    unsigned Align = std::min(Dest.second, Src.second);
    Builder.CreateMemCpy(Dest.first, Src.first, SizeVal, Align, false);
    return RValue::get(Dest.first);
  }

  case Builtin::BI__builtin_objc_memmove_collectable: {
    Value *Address = EmitScalarExpr(E->getArg(0));
    Value *SrcAddr = EmitScalarExpr(E->getArg(1));
    Value *SizeVal = EmitScalarExpr(E->getArg(2));
    CGM.getObjCRuntime().EmitGCMemmoveCollectable(*this,
                                                  Address, SrcAddr, SizeVal);
    return RValue::get(Address);
  }

  case Builtin::BI__builtin___memmove_chk: {
    // fold __builtin_memmove_chk(x, y, cst1, cst2) to memmove iff cst1<=cst2.
    llvm::APSInt Size, DstSize;
    if (!E->getArg(2)->EvaluateAsInt(Size, CGM.getContext()) ||
        !E->getArg(3)->EvaluateAsInt(DstSize, CGM.getContext()))
      break;
    if (Size.ugt(DstSize))
      break;
    std::pair<llvm::Value*, unsigned> Dest =
        EmitPointerWithAlignment(E->getArg(0));
    std::pair<llvm::Value*, unsigned> Src =
        EmitPointerWithAlignment(E->getArg(1));
    Value *SizeVal = llvm::ConstantInt::get(Builder.getContext(), Size);
    unsigned Align = std::min(Dest.second, Src.second);
    Builder.CreateMemMove(Dest.first, Src.first, SizeVal, Align, false);
    return RValue::get(Dest.first);
  }

  case Builtin::BImemmove:
  case Builtin::BI__builtin_memmove: {
    std::pair<llvm::Value*, unsigned> Dest =
        EmitPointerWithAlignment(E->getArg(0));
    std::pair<llvm::Value*, unsigned> Src =
        EmitPointerWithAlignment(E->getArg(1));
    Value *SizeVal = EmitScalarExpr(E->getArg(2));
    unsigned Align = std::min(Dest.second, Src.second);
    Builder.CreateMemMove(Dest.first, Src.first, SizeVal, Align, false);
    return RValue::get(Dest.first);
  }
  case Builtin::BImemset:
  case Builtin::BI__builtin_memset: {
    std::pair<llvm::Value*, unsigned> Dest =
        EmitPointerWithAlignment(E->getArg(0));
    Value *ByteVal = Builder.CreateTrunc(EmitScalarExpr(E->getArg(1)),
                                         Builder.getInt8Ty());
    Value *SizeVal = EmitScalarExpr(E->getArg(2));
    Builder.CreateMemSet(Dest.first, ByteVal, SizeVal, Dest.second, false);
    return RValue::get(Dest.first);
  }
  case Builtin::BI__builtin___memset_chk: {
    // fold __builtin_memset_chk(x, y, cst1, cst2) to memset iff cst1<=cst2.
    llvm::APSInt Size, DstSize;
    if (!E->getArg(2)->EvaluateAsInt(Size, CGM.getContext()) ||
        !E->getArg(3)->EvaluateAsInt(DstSize, CGM.getContext()))
      break;
    if (Size.ugt(DstSize))
      break;
    std::pair<llvm::Value*, unsigned> Dest =
        EmitPointerWithAlignment(E->getArg(0));
    Value *ByteVal = Builder.CreateTrunc(EmitScalarExpr(E->getArg(1)),
                                         Builder.getInt8Ty());
    Value *SizeVal = llvm::ConstantInt::get(Builder.getContext(), Size);
    Builder.CreateMemSet(Dest.first, ByteVal, SizeVal, Dest.second, false);
    return RValue::get(Dest.first);
  }
  case Builtin::BI__builtin_dwarf_cfa: {
    // The offset in bytes from the first argument to the CFA.
    //
    // Why on earth is this in the frontend?  Is there any reason at
    // all that the backend can't reasonably determine this while
    // lowering llvm.eh.dwarf.cfa()?
    //
    // TODO: If there's a satisfactory reason, add a target hook for
    // this instead of hard-coding 0, which is correct for most targets.
    int32_t Offset = 0;

    Value *F = CGM.getIntrinsic(Intrinsic::eh_dwarf_cfa);
    return RValue::get(Builder.CreateCall(F,
                                      llvm::ConstantInt::get(Int32Ty, Offset)));
  }
  case Builtin::BI__builtin_return_address: {
    Value *Depth = EmitScalarExpr(E->getArg(0));
    Depth = Builder.CreateIntCast(Depth, Int32Ty, false);
    Value *F = CGM.getIntrinsic(Intrinsic::returnaddress);
    return RValue::get(Builder.CreateCall(F, Depth));
  }
  case Builtin::BI__builtin_frame_address: {
    Value *Depth = EmitScalarExpr(E->getArg(0));
    Depth = Builder.CreateIntCast(Depth, Int32Ty, false);
    Value *F = CGM.getIntrinsic(Intrinsic::frameaddress);
    return RValue::get(Builder.CreateCall(F, Depth));
  }
  case Builtin::BI__builtin_extract_return_addr: {
    Value *Address = EmitScalarExpr(E->getArg(0));
    Value *Result = getTargetHooks().decodeReturnAddress(*this, Address);
    return RValue::get(Result);
  }
  case Builtin::BI__builtin_frob_return_addr: {
    Value *Address = EmitScalarExpr(E->getArg(0));
    Value *Result = getTargetHooks().encodeReturnAddress(*this, Address);
    return RValue::get(Result);
  }
  case Builtin::BI__builtin_dwarf_sp_column: {
    llvm::IntegerType *Ty
      = cast<llvm::IntegerType>(ConvertType(E->getType()));
    int Column = getTargetHooks().getDwarfEHStackPointer(CGM);
    if (Column == -1) {
      CGM.ErrorUnsupported(E, "__builtin_dwarf_sp_column");
      return RValue::get(llvm::UndefValue::get(Ty));
    }
    return RValue::get(llvm::ConstantInt::get(Ty, Column, true));
  }
  case Builtin::BI__builtin_init_dwarf_reg_size_table: {
    Value *Address = EmitScalarExpr(E->getArg(0));
    if (getTargetHooks().initDwarfEHRegSizeTable(*this, Address))
      CGM.ErrorUnsupported(E, "__builtin_init_dwarf_reg_size_table");
    return RValue::get(llvm::UndefValue::get(ConvertType(E->getType())));
  }
  case Builtin::BI__builtin_eh_return: {
    Value *Int = EmitScalarExpr(E->getArg(0));
    Value *Ptr = EmitScalarExpr(E->getArg(1));

    llvm::IntegerType *IntTy = cast<llvm::IntegerType>(Int->getType());
    assert((IntTy->getBitWidth() == 32 || IntTy->getBitWidth() == 64) &&
           "LLVM's __builtin_eh_return only supports 32- and 64-bit variants");
    Value *F = CGM.getIntrinsic(IntTy->getBitWidth() == 32
                                  ? Intrinsic::eh_return_i32
                                  : Intrinsic::eh_return_i64);
    Builder.CreateCall2(F, Int, Ptr);
    Builder.CreateUnreachable();

    // We do need to preserve an insertion point.
    EmitBlock(createBasicBlock("builtin_eh_return.cont"));

    return RValue::get(0);
  }
  case Builtin::BI__builtin_unwind_init: {
    Value *F = CGM.getIntrinsic(Intrinsic::eh_unwind_init);
    return RValue::get(Builder.CreateCall(F));
  }
  case Builtin::BI__builtin_extend_pointer: {
    // Extends a pointer to the size of an _Unwind_Word, which is
    // uint64_t on all platforms.  Generally this gets poked into a
    // register and eventually used as an address, so if the
    // addressing registers are wider than pointers and the platform
    // doesn't implicitly ignore high-order bits when doing
    // addressing, we need to make sure we zext / sext based on
    // the platform's expectations.
    //
    // See: http://gcc.gnu.org/ml/gcc-bugs/2002-02/msg00237.html

    // Cast the pointer to intptr_t.
    Value *Ptr = EmitScalarExpr(E->getArg(0));
    Value *Result = Builder.CreatePtrToInt(Ptr, IntPtrTy, "extend.cast");

    // If that's 64 bits, we're done.
    if (IntPtrTy->getBitWidth() == 64)
      return RValue::get(Result);

    // Otherwise, ask the codegen data what to do.
    if (getTargetHooks().extendPointerWithSExt())
      return RValue::get(Builder.CreateSExt(Result, Int64Ty, "extend.sext"));
    else
      return RValue::get(Builder.CreateZExt(Result, Int64Ty, "extend.zext"));
  }
  case Builtin::BI__builtin_setjmp: {
    // Buffer is a void**.
    Value *Buf = EmitScalarExpr(E->getArg(0));

    // Store the frame pointer to the setjmp buffer.
    Value *FrameAddr =
      Builder.CreateCall(CGM.getIntrinsic(Intrinsic::frameaddress),
                         ConstantInt::get(Int32Ty, 0));
    Builder.CreateStore(FrameAddr, Buf);

    // Store the stack pointer to the setjmp buffer.
    Value *StackAddr =
      Builder.CreateCall(CGM.getIntrinsic(Intrinsic::stacksave));
    Value *StackSaveSlot =
      Builder.CreateGEP(Buf, ConstantInt::get(Int32Ty, 2));
    Builder.CreateStore(StackAddr, StackSaveSlot);

    // Call LLVM's EH setjmp, which is lightweight.
    Value *F = CGM.getIntrinsic(Intrinsic::eh_sjlj_setjmp);
    Buf = Builder.CreateBitCast(Buf, Int8PtrTy);
    return RValue::get(Builder.CreateCall(F, Buf));
  }
  case Builtin::BI__builtin_longjmp: {
    Value *Buf = EmitScalarExpr(E->getArg(0));
    Buf = Builder.CreateBitCast(Buf, Int8PtrTy);

    // Call LLVM's EH longjmp, which is lightweight.
    Builder.CreateCall(CGM.getIntrinsic(Intrinsic::eh_sjlj_longjmp), Buf);

    // longjmp doesn't return; mark this as unreachable.
    Builder.CreateUnreachable();

    // We do need to preserve an insertion point.
    EmitBlock(createBasicBlock("longjmp.cont"));

    return RValue::get(0);
  }
  case Builtin::BI__sync_fetch_and_add:
  case Builtin::BI__sync_fetch_and_sub:
  case Builtin::BI__sync_fetch_and_or:
  case Builtin::BI__sync_fetch_and_and:
  case Builtin::BI__sync_fetch_and_xor:
  case Builtin::BI__sync_add_and_fetch:
  case Builtin::BI__sync_sub_and_fetch:
  case Builtin::BI__sync_and_and_fetch:
  case Builtin::BI__sync_or_and_fetch:
  case Builtin::BI__sync_xor_and_fetch:
  case Builtin::BI__sync_val_compare_and_swap:
  case Builtin::BI__sync_bool_compare_and_swap:
  case Builtin::BI__sync_lock_test_and_set:
  case Builtin::BI__sync_lock_release:
  case Builtin::BI__sync_swap:
    llvm_unreachable("Shouldn't make it through sema");
  case Builtin::BI__sync_fetch_and_add_1:
  case Builtin::BI__sync_fetch_and_add_2:
  case Builtin::BI__sync_fetch_and_add_4:
  case Builtin::BI__sync_fetch_and_add_8:
  case Builtin::BI__sync_fetch_and_add_16:
    return EmitBinaryAtomic(*this, llvm::AtomicRMWInst::Add, E);
  case Builtin::BI__sync_fetch_and_sub_1:
  case Builtin::BI__sync_fetch_and_sub_2:
  case Builtin::BI__sync_fetch_and_sub_4:
  case Builtin::BI__sync_fetch_and_sub_8:
  case Builtin::BI__sync_fetch_and_sub_16:
    return EmitBinaryAtomic(*this, llvm::AtomicRMWInst::Sub, E);
  case Builtin::BI__sync_fetch_and_or_1:
  case Builtin::BI__sync_fetch_and_or_2:
  case Builtin::BI__sync_fetch_and_or_4:
  case Builtin::BI__sync_fetch_and_or_8:
  case Builtin::BI__sync_fetch_and_or_16:
    return EmitBinaryAtomic(*this, llvm::AtomicRMWInst::Or, E);
  case Builtin::BI__sync_fetch_and_and_1:
  case Builtin::BI__sync_fetch_and_and_2:
  case Builtin::BI__sync_fetch_and_and_4:
  case Builtin::BI__sync_fetch_and_and_8:
  case Builtin::BI__sync_fetch_and_and_16:
    return EmitBinaryAtomic(*this, llvm::AtomicRMWInst::And, E);
  case Builtin::BI__sync_fetch_and_xor_1:
  case Builtin::BI__sync_fetch_and_xor_2:
  case Builtin::BI__sync_fetch_and_xor_4:
  case Builtin::BI__sync_fetch_and_xor_8:
  case Builtin::BI__sync_fetch_and_xor_16:
    return EmitBinaryAtomic(*this, llvm::AtomicRMWInst::Xor, E);

  // Clang extensions: not overloaded yet.
  case Builtin::BI__sync_fetch_and_min:
    return EmitBinaryAtomic(*this, llvm::AtomicRMWInst::Min, E);
  case Builtin::BI__sync_fetch_and_max:
    return EmitBinaryAtomic(*this, llvm::AtomicRMWInst::Max, E);
  case Builtin::BI__sync_fetch_and_umin:
    return EmitBinaryAtomic(*this, llvm::AtomicRMWInst::UMin, E);
  case Builtin::BI__sync_fetch_and_umax:
    return EmitBinaryAtomic(*this, llvm::AtomicRMWInst::UMax, E);

  case Builtin::BI__sync_add_and_fetch_1:
  case Builtin::BI__sync_add_and_fetch_2:
  case Builtin::BI__sync_add_and_fetch_4:
  case Builtin::BI__sync_add_and_fetch_8:
  case Builtin::BI__sync_add_and_fetch_16:
    return EmitBinaryAtomicPost(*this, llvm::AtomicRMWInst::Add, E,
                                llvm::Instruction::Add);
  case Builtin::BI__sync_sub_and_fetch_1:
  case Builtin::BI__sync_sub_and_fetch_2:
  case Builtin::BI__sync_sub_and_fetch_4:
  case Builtin::BI__sync_sub_and_fetch_8:
  case Builtin::BI__sync_sub_and_fetch_16:
    return EmitBinaryAtomicPost(*this, llvm::AtomicRMWInst::Sub, E,
                                llvm::Instruction::Sub);
  case Builtin::BI__sync_and_and_fetch_1:
  case Builtin::BI__sync_and_and_fetch_2:
  case Builtin::BI__sync_and_and_fetch_4:
  case Builtin::BI__sync_and_and_fetch_8:
  case Builtin::BI__sync_and_and_fetch_16:
    return EmitBinaryAtomicPost(*this, llvm::AtomicRMWInst::And, E,
                                llvm::Instruction::And);
  case Builtin::BI__sync_or_and_fetch_1:
  case Builtin::BI__sync_or_and_fetch_2:
  case Builtin::BI__sync_or_and_fetch_4:
  case Builtin::BI__sync_or_and_fetch_8:
  case Builtin::BI__sync_or_and_fetch_16:
    return EmitBinaryAtomicPost(*this, llvm::AtomicRMWInst::Or, E,
                                llvm::Instruction::Or);
  case Builtin::BI__sync_xor_and_fetch_1:
  case Builtin::BI__sync_xor_and_fetch_2:
  case Builtin::BI__sync_xor_and_fetch_4:
  case Builtin::BI__sync_xor_and_fetch_8:
  case Builtin::BI__sync_xor_and_fetch_16:
    return EmitBinaryAtomicPost(*this, llvm::AtomicRMWInst::Xor, E,
                                llvm::Instruction::Xor);

  case Builtin::BI__sync_val_compare_and_swap_1:
  case Builtin::BI__sync_val_compare_and_swap_2:
  case Builtin::BI__sync_val_compare_and_swap_4:
  case Builtin::BI__sync_val_compare_and_swap_8:
  case Builtin::BI__sync_val_compare_and_swap_16: {
    QualType T = E->getType();
    llvm::Value *DestPtr = EmitScalarExpr(E->getArg(0));
    unsigned AddrSpace = DestPtr->getType()->getPointerAddressSpace();

    llvm::IntegerType *IntType =
      llvm::IntegerType::get(getLLVMContext(),
                             getContext().getTypeSize(T));
    llvm::Type *IntPtrType = IntType->getPointerTo(AddrSpace);

    Value *Args[3];
    Args[0] = Builder.CreateBitCast(DestPtr, IntPtrType);
    Args[1] = EmitScalarExpr(E->getArg(1));
    llvm::Type *ValueType = Args[1]->getType();
    Args[1] = EmitToInt(*this, Args[1], T, IntType);
    Args[2] = EmitToInt(*this, EmitScalarExpr(E->getArg(2)), T, IntType);

    Value *Result = Builder.CreateAtomicCmpXchg(Args[0], Args[1], Args[2],
                                                llvm::SequentiallyConsistent);
    Result = EmitFromInt(*this, Result, T, ValueType);
    return RValue::get(Result);
  }

  case Builtin::BI__sync_bool_compare_and_swap_1:
  case Builtin::BI__sync_bool_compare_and_swap_2:
  case Builtin::BI__sync_bool_compare_and_swap_4:
  case Builtin::BI__sync_bool_compare_and_swap_8:
  case Builtin::BI__sync_bool_compare_and_swap_16: {
    QualType T = E->getArg(1)->getType();
    llvm::Value *DestPtr = EmitScalarExpr(E->getArg(0));
    unsigned AddrSpace = DestPtr->getType()->getPointerAddressSpace();

    llvm::IntegerType *IntType =
      llvm::IntegerType::get(getLLVMContext(),
                             getContext().getTypeSize(T));
    llvm::Type *IntPtrType = IntType->getPointerTo(AddrSpace);

    Value *Args[3];
    Args[0] = Builder.CreateBitCast(DestPtr, IntPtrType);
    Args[1] = EmitToInt(*this, EmitScalarExpr(E->getArg(1)), T, IntType);
    Args[2] = EmitToInt(*this, EmitScalarExpr(E->getArg(2)), T, IntType);

    Value *OldVal = Args[1];
    Value *PrevVal = Builder.CreateAtomicCmpXchg(Args[0], Args[1], Args[2],
                                                 llvm::SequentiallyConsistent);
    Value *Result = Builder.CreateICmpEQ(PrevVal, OldVal);
    // zext bool to int.
    Result = Builder.CreateZExt(Result, ConvertType(E->getType()));
    return RValue::get(Result);
  }

  case Builtin::BI__sync_swap_1:
  case Builtin::BI__sync_swap_2:
  case Builtin::BI__sync_swap_4:
  case Builtin::BI__sync_swap_8:
  case Builtin::BI__sync_swap_16:
    return EmitBinaryAtomic(*this, llvm::AtomicRMWInst::Xchg, E);

  case Builtin::BI__sync_lock_test_and_set_1:
  case Builtin::BI__sync_lock_test_and_set_2:
  case Builtin::BI__sync_lock_test_and_set_4:
  case Builtin::BI__sync_lock_test_and_set_8:
  case Builtin::BI__sync_lock_test_and_set_16:
    return EmitBinaryAtomic(*this, llvm::AtomicRMWInst::Xchg, E);

  case Builtin::BI__sync_lock_release_1:
  case Builtin::BI__sync_lock_release_2:
  case Builtin::BI__sync_lock_release_4:
  case Builtin::BI__sync_lock_release_8:
  case Builtin::BI__sync_lock_release_16: {
    Value *Ptr = EmitScalarExpr(E->getArg(0));
    QualType ElTy = E->getArg(0)->getType()->getPointeeType();
    CharUnits StoreSize = getContext().getTypeSizeInChars(ElTy);
    llvm::Type *ITy = llvm::IntegerType::get(getLLVMContext(),
                                             StoreSize.getQuantity() * 8);
    Ptr = Builder.CreateBitCast(Ptr, ITy->getPointerTo());
    llvm::StoreInst *Store =
      Builder.CreateStore(llvm::Constant::getNullValue(ITy), Ptr);
    Store->setAlignment(StoreSize.getQuantity());
    Store->setAtomic(llvm::Release);
    return RValue::get(0);
  }

  case Builtin::BI__sync_synchronize: {
    // We assume this is supposed to correspond to a C++0x-style
    // sequentially-consistent fence (i.e. this is only usable for
    // synchonization, not device I/O or anything like that). This intrinsic
    // is really badly designed in the sense that in theory, there isn't
    // any way to safely use it... but in practice, it mostly works
    // to use it with non-atomic loads and stores to get acquire/release
    // semantics.
    Builder.CreateFence(llvm::SequentiallyConsistent);
    return RValue::get(0);
  }

  case Builtin::BI__c11_atomic_is_lock_free:
  case Builtin::BI__atomic_is_lock_free: {
    // Call "bool __atomic_is_lock_free(size_t size, void *ptr)". For the
    // __c11 builtin, ptr is 0 (indicating a properly-aligned object), since
    // _Atomic(T) is always properly-aligned.
    const char *LibCallName = "__atomic_is_lock_free";
    CallArgList Args;
    Args.add(RValue::get(EmitScalarExpr(E->getArg(0))),
             getContext().getSizeType());
    if (BuiltinID == Builtin::BI__atomic_is_lock_free)
      Args.add(RValue::get(EmitScalarExpr(E->getArg(1))),
               getContext().VoidPtrTy);
    else
      Args.add(RValue::get(llvm::Constant::getNullValue(VoidPtrTy)),
               getContext().VoidPtrTy);
    const CGFunctionInfo &FuncInfo =
        CGM.getTypes().arrangeFreeFunctionCall(E->getType(), Args,
                                               FunctionType::ExtInfo(),
                                               RequiredArgs::All);
    llvm::FunctionType *FTy = CGM.getTypes().GetFunctionType(FuncInfo);
    llvm::Constant *Func = CGM.CreateRuntimeFunction(FTy, LibCallName);
    return EmitCall(FuncInfo, Func, ReturnValueSlot(), Args);
  }

  case Builtin::BI__atomic_test_and_set: {
    // Look at the argument type to determine whether this is a volatile
    // operation. The parameter type is always volatile.
    QualType PtrTy = E->getArg(0)->IgnoreImpCasts()->getType();
    bool Volatile =
        PtrTy->castAs<PointerType>()->getPointeeType().isVolatileQualified();

    Value *Ptr = EmitScalarExpr(E->getArg(0));
    unsigned AddrSpace = Ptr->getType()->getPointerAddressSpace();
    Ptr = Builder.CreateBitCast(Ptr, Int8Ty->getPointerTo(AddrSpace));
    Value *NewVal = Builder.getInt8(1);
    Value *Order = EmitScalarExpr(E->getArg(1));
    if (isa<llvm::ConstantInt>(Order)) {
      int ord = cast<llvm::ConstantInt>(Order)->getZExtValue();
      AtomicRMWInst *Result = 0;
      switch (ord) {
      case 0:  // memory_order_relaxed
      default: // invalid order
        Result = Builder.CreateAtomicRMW(llvm::AtomicRMWInst::Xchg,
                                         Ptr, NewVal,
                                         llvm::Monotonic);
        break;
      case 1:  // memory_order_consume
      case 2:  // memory_order_acquire
        Result = Builder.CreateAtomicRMW(llvm::AtomicRMWInst::Xchg,
                                         Ptr, NewVal,
                                         llvm::Acquire);
        break;
      case 3:  // memory_order_release
        Result = Builder.CreateAtomicRMW(llvm::AtomicRMWInst::Xchg,
                                         Ptr, NewVal,
                                         llvm::Release);
        break;
      case 4:  // memory_order_acq_rel
        Result = Builder.CreateAtomicRMW(llvm::AtomicRMWInst::Xchg,
                                         Ptr, NewVal,
                                         llvm::AcquireRelease);
        break;
      case 5:  // memory_order_seq_cst
        Result = Builder.CreateAtomicRMW(llvm::AtomicRMWInst::Xchg,
                                         Ptr, NewVal,
                                         llvm::SequentiallyConsistent);
        break;
      }
      Result->setVolatile(Volatile);
      return RValue::get(Builder.CreateIsNotNull(Result, "tobool"));
    }

    llvm::BasicBlock *ContBB = createBasicBlock("atomic.continue", CurFn);

    llvm::BasicBlock *BBs[5] = {
      createBasicBlock("monotonic", CurFn),
      createBasicBlock("acquire", CurFn),
      createBasicBlock("release", CurFn),
      createBasicBlock("acqrel", CurFn),
      createBasicBlock("seqcst", CurFn)
    };
    llvm::AtomicOrdering Orders[5] = {
      llvm::Monotonic, llvm::Acquire, llvm::Release,
      llvm::AcquireRelease, llvm::SequentiallyConsistent
    };

    Order = Builder.CreateIntCast(Order, Builder.getInt32Ty(), false);
    llvm::SwitchInst *SI = Builder.CreateSwitch(Order, BBs[0]);

    Builder.SetInsertPoint(ContBB);
    PHINode *Result = Builder.CreatePHI(Int8Ty, 5, "was_set");

    for (unsigned i = 0; i < 5; ++i) {
      Builder.SetInsertPoint(BBs[i]);
      AtomicRMWInst *RMW = Builder.CreateAtomicRMW(llvm::AtomicRMWInst::Xchg,
                                                   Ptr, NewVal, Orders[i]);
      RMW->setVolatile(Volatile);
      Result->addIncoming(RMW, BBs[i]);
      Builder.CreateBr(ContBB);
    }

    SI->addCase(Builder.getInt32(0), BBs[0]);
    SI->addCase(Builder.getInt32(1), BBs[1]);
    SI->addCase(Builder.getInt32(2), BBs[1]);
    SI->addCase(Builder.getInt32(3), BBs[2]);
    SI->addCase(Builder.getInt32(4), BBs[3]);
    SI->addCase(Builder.getInt32(5), BBs[4]);

    Builder.SetInsertPoint(ContBB);
    return RValue::get(Builder.CreateIsNotNull(Result, "tobool"));
  }

  case Builtin::BI__atomic_clear: {
    QualType PtrTy = E->getArg(0)->IgnoreImpCasts()->getType();
    bool Volatile =
        PtrTy->castAs<PointerType>()->getPointeeType().isVolatileQualified();

    Value *Ptr = EmitScalarExpr(E->getArg(0));
    unsigned AddrSpace = Ptr->getType()->getPointerAddressSpace();
    Ptr = Builder.CreateBitCast(Ptr, Int8Ty->getPointerTo(AddrSpace));
    Value *NewVal = Builder.getInt8(0);
    Value *Order = EmitScalarExpr(E->getArg(1));
    if (isa<llvm::ConstantInt>(Order)) {
      int ord = cast<llvm::ConstantInt>(Order)->getZExtValue();
      StoreInst *Store = Builder.CreateStore(NewVal, Ptr, Volatile);
      Store->setAlignment(1);
      switch (ord) {
      case 0:  // memory_order_relaxed
      default: // invalid order
        Store->setOrdering(llvm::Monotonic);
        break;
      case 3:  // memory_order_release
        Store->setOrdering(llvm::Release);
        break;
      case 5:  // memory_order_seq_cst
        Store->setOrdering(llvm::SequentiallyConsistent);
        break;
      }
      return RValue::get(0);
    }

    llvm::BasicBlock *ContBB = createBasicBlock("atomic.continue", CurFn);

    llvm::BasicBlock *BBs[3] = {
      createBasicBlock("monotonic", CurFn),
      createBasicBlock("release", CurFn),
      createBasicBlock("seqcst", CurFn)
    };
    llvm::AtomicOrdering Orders[3] = {
      llvm::Monotonic, llvm::Release, llvm::SequentiallyConsistent
    };

    Order = Builder.CreateIntCast(Order, Builder.getInt32Ty(), false);
    llvm::SwitchInst *SI = Builder.CreateSwitch(Order, BBs[0]);

    for (unsigned i = 0; i < 3; ++i) {
      Builder.SetInsertPoint(BBs[i]);
      StoreInst *Store = Builder.CreateStore(NewVal, Ptr, Volatile);
      Store->setAlignment(1);
      Store->setOrdering(Orders[i]);
      Builder.CreateBr(ContBB);
    }

    SI->addCase(Builder.getInt32(0), BBs[0]);
    SI->addCase(Builder.getInt32(3), BBs[1]);
    SI->addCase(Builder.getInt32(5), BBs[2]);

    Builder.SetInsertPoint(ContBB);
    return RValue::get(0);
  }

  case Builtin::BI__atomic_thread_fence:
  case Builtin::BI__atomic_signal_fence:
  case Builtin::BI__c11_atomic_thread_fence:
  case Builtin::BI__c11_atomic_signal_fence: {
    llvm::SynchronizationScope Scope;
    if (BuiltinID == Builtin::BI__atomic_signal_fence ||
        BuiltinID == Builtin::BI__c11_atomic_signal_fence)
      Scope = llvm::SingleThread;
    else
      Scope = llvm::CrossThread;
    Value *Order = EmitScalarExpr(E->getArg(0));
    if (isa<llvm::ConstantInt>(Order)) {
      int ord = cast<llvm::ConstantInt>(Order)->getZExtValue();
      switch (ord) {
      case 0:  // memory_order_relaxed
      default: // invalid order
        break;
      case 1:  // memory_order_consume
      case 2:  // memory_order_acquire
        Builder.CreateFence(llvm::Acquire, Scope);
        break;
      case 3:  // memory_order_release
        Builder.CreateFence(llvm::Release, Scope);
        break;
      case 4:  // memory_order_acq_rel
        Builder.CreateFence(llvm::AcquireRelease, Scope);
        break;
      case 5:  // memory_order_seq_cst
        Builder.CreateFence(llvm::SequentiallyConsistent, Scope);
        break;
      }
      return RValue::get(0);
    }

    llvm::BasicBlock *AcquireBB, *ReleaseBB, *AcqRelBB, *SeqCstBB;
    AcquireBB = createBasicBlock("acquire", CurFn);
    ReleaseBB = createBasicBlock("release", CurFn);
    AcqRelBB = createBasicBlock("acqrel", CurFn);
    SeqCstBB = createBasicBlock("seqcst", CurFn);
    llvm::BasicBlock *ContBB = createBasicBlock("atomic.continue", CurFn);

    Order = Builder.CreateIntCast(Order, Builder.getInt32Ty(), false);
    llvm::SwitchInst *SI = Builder.CreateSwitch(Order, ContBB);

    Builder.SetInsertPoint(AcquireBB);
    Builder.CreateFence(llvm::Acquire, Scope);
    Builder.CreateBr(ContBB);
    SI->addCase(Builder.getInt32(1), AcquireBB);
    SI->addCase(Builder.getInt32(2), AcquireBB);

    Builder.SetInsertPoint(ReleaseBB);
    Builder.CreateFence(llvm::Release, Scope);
    Builder.CreateBr(ContBB);
    SI->addCase(Builder.getInt32(3), ReleaseBB);

    Builder.SetInsertPoint(AcqRelBB);
    Builder.CreateFence(llvm::AcquireRelease, Scope);
    Builder.CreateBr(ContBB);
    SI->addCase(Builder.getInt32(4), AcqRelBB);

    Builder.SetInsertPoint(SeqCstBB);
    Builder.CreateFence(llvm::SequentiallyConsistent, Scope);
    Builder.CreateBr(ContBB);
    SI->addCase(Builder.getInt32(5), SeqCstBB);

    Builder.SetInsertPoint(ContBB);
    return RValue::get(0);
  }

    // Library functions with special handling.
  case Builtin::BIsqrt:
  case Builtin::BIsqrtf:
  case Builtin::BIsqrtl: {
    // Transform a call to sqrt* into a @llvm.sqrt.* intrinsic call, but only
    // in finite- or unsafe-math mode (the intrinsic has different semantics
    // for handling negative numbers compared to the library function, so
    // -fmath-errno=0 is not enough).
    if (!FD->hasAttr<ConstAttr>())
      break;
    if (!(CGM.getCodeGenOpts().UnsafeFPMath ||
          CGM.getCodeGenOpts().NoNaNsFPMath))
      break;
    Value *Arg0 = EmitScalarExpr(E->getArg(0));
    llvm::Type *ArgType = Arg0->getType();
    Value *F = CGM.getIntrinsic(Intrinsic::sqrt, ArgType);
    return RValue::get(Builder.CreateCall(F, Arg0));
  }

  case Builtin::BIpow:
  case Builtin::BIpowf:
  case Builtin::BIpowl: {
    // Transform a call to pow* into a @llvm.pow.* intrinsic call.
    if (!FD->hasAttr<ConstAttr>())
      break;
    Value *Base = EmitScalarExpr(E->getArg(0));
    Value *Exponent = EmitScalarExpr(E->getArg(1));
    llvm::Type *ArgType = Base->getType();
    Value *F = CGM.getIntrinsic(Intrinsic::pow, ArgType);
    return RValue::get(Builder.CreateCall2(F, Base, Exponent));
    break;
  }

  case Builtin::BIfma:
  case Builtin::BIfmaf:
  case Builtin::BIfmal:
  case Builtin::BI__builtin_fma:
  case Builtin::BI__builtin_fmaf:
  case Builtin::BI__builtin_fmal: {
    // Rewrite fma to intrinsic.
    Value *FirstArg = EmitScalarExpr(E->getArg(0));
    llvm::Type *ArgType = FirstArg->getType();
    Value *F = CGM.getIntrinsic(Intrinsic::fma, ArgType);
    return RValue::get(Builder.CreateCall3(F, FirstArg,
                                              EmitScalarExpr(E->getArg(1)),
                                              EmitScalarExpr(E->getArg(2))));
  }

  case Builtin::BI__builtin_signbit:
  case Builtin::BI__builtin_signbitf:
  case Builtin::BI__builtin_signbitl: {
    LLVMContext &C = CGM.getLLVMContext();

    Value *Arg = EmitScalarExpr(E->getArg(0));
    llvm::Type *ArgTy = Arg->getType();
    if (ArgTy->isPPC_FP128Ty())
      break; // FIXME: I'm not sure what the right implementation is here.
    int ArgWidth = ArgTy->getPrimitiveSizeInBits();
    llvm::Type *ArgIntTy = llvm::IntegerType::get(C, ArgWidth);
    Value *BCArg = Builder.CreateBitCast(Arg, ArgIntTy);
    Value *ZeroCmp = llvm::Constant::getNullValue(ArgIntTy);
    Value *Result = Builder.CreateICmpSLT(BCArg, ZeroCmp);
    return RValue::get(Builder.CreateZExt(Result, ConvertType(E->getType())));
  }
  case Builtin::BI__builtin_annotation: {
    llvm::Value *AnnVal = EmitScalarExpr(E->getArg(0));
    llvm::Value *F = CGM.getIntrinsic(llvm::Intrinsic::annotation,
                                      AnnVal->getType());

    // Get the annotation string, go through casts. Sema requires this to be a
    // non-wide string literal, potentially casted, so the cast<> is safe.
    const Expr *AnnotationStrExpr = E->getArg(1)->IgnoreParenCasts();
    StringRef Str = cast<StringLiteral>(AnnotationStrExpr)->getString();
    return RValue::get(EmitAnnotationCall(F, AnnVal, Str, E->getExprLoc()));
  }
  case Builtin::BI__builtin_addcb:
  case Builtin::BI__builtin_addcs:
  case Builtin::BI__builtin_addc:
  case Builtin::BI__builtin_addcl:
  case Builtin::BI__builtin_addcll:
  case Builtin::BI__builtin_subcb:
  case Builtin::BI__builtin_subcs:
  case Builtin::BI__builtin_subc:
  case Builtin::BI__builtin_subcl:
  case Builtin::BI__builtin_subcll: {

    // We translate all of these builtins from expressions of the form:
    //   int x = ..., y = ..., carryin = ..., carryout, result;
    //   result = __builtin_addc(x, y, carryin, &carryout);
    //
    // to LLVM IR of the form:
    //
    //   %tmp1 = call {i32, i1} @llvm.uadd.with.overflow.i32(i32 %x, i32 %y)
    //   %tmpsum1 = extractvalue {i32, i1} %tmp1, 0
    //   %carry1 = extractvalue {i32, i1} %tmp1, 1
    //   %tmp2 = call {i32, i1} @llvm.uadd.with.overflow.i32(i32 %tmpsum1,
    //                                                       i32 %carryin)
    //   %result = extractvalue {i32, i1} %tmp2, 0
    //   %carry2 = extractvalue {i32, i1} %tmp2, 1
    //   %tmp3 = or i1 %carry1, %carry2
    //   %tmp4 = zext i1 %tmp3 to i32
    //   store i32 %tmp4, i32* %carryout

    // Scalarize our inputs.
    llvm::Value *X = EmitScalarExpr(E->getArg(0));
    llvm::Value *Y = EmitScalarExpr(E->getArg(1));
    llvm::Value *Carryin = EmitScalarExpr(E->getArg(2));
    std::pair<llvm::Value*, unsigned> CarryOutPtr =
      EmitPointerWithAlignment(E->getArg(3));

    // Decide if we are lowering to a uadd.with.overflow or usub.with.overflow.
    llvm::Intrinsic::ID IntrinsicId;
    switch (BuiltinID) {
    default: llvm_unreachable("Unknown multiprecision builtin id.");
    case Builtin::BI__builtin_addcb:
    case Builtin::BI__builtin_addcs:
    case Builtin::BI__builtin_addc:
    case Builtin::BI__builtin_addcl:
    case Builtin::BI__builtin_addcll:
      IntrinsicId = llvm::Intrinsic::uadd_with_overflow;
      break;
    case Builtin::BI__builtin_subcb:
    case Builtin::BI__builtin_subcs:
    case Builtin::BI__builtin_subc:
    case Builtin::BI__builtin_subcl:
    case Builtin::BI__builtin_subcll:
      IntrinsicId = llvm::Intrinsic::usub_with_overflow;
      break;
    }

    // Construct our resulting LLVM IR expression.
    llvm::Value *Carry1;
    llvm::Value *Sum1 = EmitOverflowIntrinsic(*this, IntrinsicId,
                                              X, Y, Carry1);
    llvm::Value *Carry2;
    llvm::Value *Sum2 = EmitOverflowIntrinsic(*this, IntrinsicId,
                                              Sum1, Carryin, Carry2);
    llvm::Value *CarryOut = Builder.CreateZExt(Builder.CreateOr(Carry1, Carry2),
                                               X->getType());
    llvm::StoreInst *CarryOutStore = Builder.CreateStore(CarryOut,
                                                         CarryOutPtr.first);
    CarryOutStore->setAlignment(CarryOutPtr.second);
    return RValue::get(Sum2);
  }
  case Builtin::BI__builtin_uadd_overflow:
  case Builtin::BI__builtin_uaddl_overflow:
  case Builtin::BI__builtin_uaddll_overflow:
  case Builtin::BI__builtin_usub_overflow:
  case Builtin::BI__builtin_usubl_overflow:
  case Builtin::BI__builtin_usubll_overflow:
  case Builtin::BI__builtin_umul_overflow:
  case Builtin::BI__builtin_umull_overflow:
  case Builtin::BI__builtin_umulll_overflow:
  case Builtin::BI__builtin_sadd_overflow:
  case Builtin::BI__builtin_saddl_overflow:
  case Builtin::BI__builtin_saddll_overflow:
  case Builtin::BI__builtin_ssub_overflow:
  case Builtin::BI__builtin_ssubl_overflow:
  case Builtin::BI__builtin_ssubll_overflow:
  case Builtin::BI__builtin_smul_overflow:
  case Builtin::BI__builtin_smull_overflow:
  case Builtin::BI__builtin_smulll_overflow: {

    // We translate all of these builtins directly to the relevant llvm IR node.

    // Scalarize our inputs.
    llvm::Value *X = EmitScalarExpr(E->getArg(0));
    llvm::Value *Y = EmitScalarExpr(E->getArg(1));
    std::pair<llvm::Value *, unsigned> SumOutPtr =
      EmitPointerWithAlignment(E->getArg(2));

    // Decide which of the overflow intrinsics we are lowering to:
    llvm::Intrinsic::ID IntrinsicId;
    switch (BuiltinID) {
    default: llvm_unreachable("Unknown security overflow builtin id.");
    case Builtin::BI__builtin_uadd_overflow:
    case Builtin::BI__builtin_uaddl_overflow:
    case Builtin::BI__builtin_uaddll_overflow:
      IntrinsicId = llvm::Intrinsic::uadd_with_overflow;
      break;
    case Builtin::BI__builtin_usub_overflow:
    case Builtin::BI__builtin_usubl_overflow:
    case Builtin::BI__builtin_usubll_overflow:
      IntrinsicId = llvm::Intrinsic::usub_with_overflow;
      break;
    case Builtin::BI__builtin_umul_overflow:
    case Builtin::BI__builtin_umull_overflow:
    case Builtin::BI__builtin_umulll_overflow:
      IntrinsicId = llvm::Intrinsic::umul_with_overflow;
      break;
    case Builtin::BI__builtin_sadd_overflow:
    case Builtin::BI__builtin_saddl_overflow:
    case Builtin::BI__builtin_saddll_overflow:
      IntrinsicId = llvm::Intrinsic::sadd_with_overflow;
      break;
    case Builtin::BI__builtin_ssub_overflow:
    case Builtin::BI__builtin_ssubl_overflow:
    case Builtin::BI__builtin_ssubll_overflow:
      IntrinsicId = llvm::Intrinsic::ssub_with_overflow;
      break;
    case Builtin::BI__builtin_smul_overflow:
    case Builtin::BI__builtin_smull_overflow:
    case Builtin::BI__builtin_smulll_overflow:
      IntrinsicId = llvm::Intrinsic::smul_with_overflow;
      break;
    }

    
    llvm::Value *Carry;
    llvm::Value *Sum = EmitOverflowIntrinsic(*this, IntrinsicId, X, Y, Carry);
    llvm::StoreInst *SumOutStore = Builder.CreateStore(Sum, SumOutPtr.first);
    SumOutStore->setAlignment(SumOutPtr.second);

    return RValue::get(Carry);
  }
  case Builtin::BI__builtin_addressof:
    return RValue::get(EmitLValue(E->getArg(0)).getAddress());
  case Builtin::BI__noop:
    return RValue::get(0);
  }

  // If this is an alias for a lib function (e.g. __builtin_sin), emit
  // the call using the normal call path, but using the unmangled
  // version of the function name.
  if (getContext().BuiltinInfo.isLibFunction(BuiltinID))
    return emitLibraryCall(*this, FD, E,
                           CGM.getBuiltinLibFunction(FD, BuiltinID));

  // If this is a predefined lib function (e.g. malloc), emit the call
  // using exactly the normal call path.
  if (getContext().BuiltinInfo.isPredefinedLibFunction(BuiltinID))
    return emitLibraryCall(*this, FD, E, EmitScalarExpr(E->getCallee()));

  // See if we have a target specific intrinsic.
  const char *Name = getContext().BuiltinInfo.GetName(BuiltinID);
  Intrinsic::ID IntrinsicID = Intrinsic::not_intrinsic;
  if (const char *Prefix =
      llvm::Triple::getArchTypePrefix(getTarget().getTriple().getArch()))
    IntrinsicID = Intrinsic::getIntrinsicForGCCBuiltin(Prefix, Name);

  if (IntrinsicID != Intrinsic::not_intrinsic) {
    SmallVector<Value*, 16> Args;

    // Find out if any arguments are required to be integer constant
    // expressions.
    unsigned ICEArguments = 0;
    ASTContext::GetBuiltinTypeError Error;
    getContext().GetBuiltinType(BuiltinID, Error, &ICEArguments);
    assert(Error == ASTContext::GE_None && "Should not codegen an error");

    Function *F = CGM.getIntrinsic(IntrinsicID);
    llvm::FunctionType *FTy = F->getFunctionType();

    for (unsigned i = 0, e = E->getNumArgs(); i != e; ++i) {
      Value *ArgValue;
      // If this is a normal argument, just emit it as a scalar.
      if ((ICEArguments & (1 << i)) == 0) {
        ArgValue = EmitScalarExpr(E->getArg(i));
      } else {
        // If this is required to be a constant, constant fold it so that we
        // know that the generated intrinsic gets a ConstantInt.
        llvm::APSInt Result;
        bool IsConst = E->getArg(i)->isIntegerConstantExpr(Result,getContext());
        assert(IsConst && "Constant arg isn't actually constant?");
        (void)IsConst;
        ArgValue = llvm::ConstantInt::get(getLLVMContext(), Result);
      }

      // If the intrinsic arg type is different from the builtin arg type
      // we need to do a bit cast.
      llvm::Type *PTy = FTy->getParamType(i);
      if (PTy != ArgValue->getType()) {
        assert(PTy->canLosslesslyBitCastTo(FTy->getParamType(i)) &&
               "Must be able to losslessly bit cast to param");
        ArgValue = Builder.CreateBitCast(ArgValue, PTy);
      }

      Args.push_back(ArgValue);
    }

    Value *V = Builder.CreateCall(F, Args);
    QualType BuiltinRetType = E->getType();

    llvm::Type *RetTy = VoidTy;
    if (!BuiltinRetType->isVoidType())
      RetTy = ConvertType(BuiltinRetType);

    if (RetTy != V->getType()) {
      assert(V->getType()->canLosslesslyBitCastTo(RetTy) &&
             "Must be able to losslessly bit cast result type");
      V = Builder.CreateBitCast(V, RetTy);
    }

    return RValue::get(V);
  }

  // See if we have a target specific builtin that needs to be lowered.
  if (Value *V = EmitTargetBuiltinExpr(BuiltinID, E))
    return RValue::get(V);

  ErrorUnsupported(E, "builtin function");

  // Unknown builtin, for now just dump it out and return undef.
  return GetUndefRValue(E->getType());
}

Value *CodeGenFunction::EmitTargetBuiltinExpr(unsigned BuiltinID,
                                              const CallExpr *E) {
  switch (getTarget().getTriple().getArch()) {
  case llvm::Triple::aarch64:
    return EmitAArch64BuiltinExpr(BuiltinID, E);
  case llvm::Triple::arm:
  case llvm::Triple::thumb:
    return EmitARMBuiltinExpr(BuiltinID, E);
  case llvm::Triple::arm64:
    return EmitARM64BuiltinExpr(BuiltinID, E);
  case llvm::Triple::x86:
  case llvm::Triple::x86_64:
    return EmitX86BuiltinExpr(BuiltinID, E);
  case llvm::Triple::ppc:
  case llvm::Triple::ppc64:
  case llvm::Triple::ppc64le:
    return EmitPPCBuiltinExpr(BuiltinID, E);
  default:
    return 0;
  }
}

static llvm::VectorType *GetNeonType(CodeGenFunction *CGF,
                                     NeonTypeFlags TypeFlags,
                                     bool V1Ty=false) {
  int IsQuad = TypeFlags.isQuad();
  switch (TypeFlags.getEltType()) {
  case NeonTypeFlags::Int8:
  case NeonTypeFlags::Poly8:
    return llvm::VectorType::get(CGF->Int8Ty, V1Ty ? 1 : (8 << IsQuad));
  case NeonTypeFlags::Int16:
  case NeonTypeFlags::Poly16:
  case NeonTypeFlags::Float16:
    return llvm::VectorType::get(CGF->Int16Ty, V1Ty ? 1 : (4 << IsQuad));
  case NeonTypeFlags::Int32:
    return llvm::VectorType::get(CGF->Int32Ty, V1Ty ? 1 : (2 << IsQuad));
  case NeonTypeFlags::Int64:
  case NeonTypeFlags::Poly64:
    return llvm::VectorType::get(CGF->Int64Ty, V1Ty ? 1 : (1 << IsQuad));
  case NeonTypeFlags::Poly128:
    // FIXME: i128 and f128 doesn't get fully support in Clang and llvm.
    // There is a lot of i128 and f128 API missing.
    // so we use v16i8 to represent poly128 and get pattern matched.
    return llvm::VectorType::get(CGF->Int8Ty, 16);
  case NeonTypeFlags::Float32:
    return llvm::VectorType::get(CGF->FloatTy, V1Ty ? 1 : (2 << IsQuad));
  case NeonTypeFlags::Float64:
    return llvm::VectorType::get(CGF->DoubleTy, V1Ty ? 1 : (1 << IsQuad));
  }
  llvm_unreachable("Unknown vector element type!");
}

Value *CodeGenFunction::EmitNeonSplat(Value *V, Constant *C) {
  unsigned nElts = cast<llvm::VectorType>(V->getType())->getNumElements();
  Value* SV = llvm::ConstantVector::getSplat(nElts, C);
  return Builder.CreateShuffleVector(V, V, SV, "lane");
}

Value *CodeGenFunction::EmitNeonCall(Function *F, SmallVectorImpl<Value*> &Ops,
                                     const char *name,
                                     unsigned shift, bool rightshift) {
  unsigned j = 0;
  for (Function::const_arg_iterator ai = F->arg_begin(), ae = F->arg_end();
       ai != ae; ++ai, ++j)
    if (shift > 0 && shift == j)
      Ops[j] = EmitNeonShiftVector(Ops[j], ai->getType(), rightshift);
    else
      Ops[j] = Builder.CreateBitCast(Ops[j], ai->getType(), name);

  return Builder.CreateCall(F, Ops, name);
}

Value *CodeGenFunction::EmitNeonShiftVector(Value *V, llvm::Type *Ty,
                                            bool neg) {
  int SV = cast<ConstantInt>(V)->getSExtValue();

  llvm::VectorType *VTy = cast<llvm::VectorType>(Ty);
  llvm::Constant *C = ConstantInt::get(VTy->getElementType(), neg ? -SV : SV);
  return llvm::ConstantVector::getSplat(VTy->getNumElements(), C);
}

// \brief Right-shift a vector by a constant.
Value *CodeGenFunction::EmitNeonRShiftImm(Value *Vec, Value *Shift,
                                          llvm::Type *Ty, bool usgn,
                                          const char *name) {
  llvm::VectorType *VTy = cast<llvm::VectorType>(Ty);

  int ShiftAmt = cast<ConstantInt>(Shift)->getSExtValue();
  int EltSize = VTy->getScalarSizeInBits();

  Vec = Builder.CreateBitCast(Vec, Ty);

  // lshr/ashr are undefined when the shift amount is equal to the vector
  // element size.
  if (ShiftAmt == EltSize) {
    if (usgn) {
      // Right-shifting an unsigned value by its size yields 0.
      llvm::Constant *Zero = ConstantInt::get(VTy->getElementType(), 0);
      return llvm::ConstantVector::getSplat(VTy->getNumElements(), Zero);
    } else {
      // Right-shifting a signed value by its size is equivalent
      // to a shift of size-1.
      --ShiftAmt;
      Shift = ConstantInt::get(VTy->getElementType(), ShiftAmt);
    }
  }

  Shift = EmitNeonShiftVector(Shift, Ty, false);
  if (usgn)
    return Builder.CreateLShr(Vec, Shift, name);
  else
    return Builder.CreateAShr(Vec, Shift, name);
}

Value *CodeGenFunction::EmitConcatVectors(Value *Lo, Value *Hi,
                                          llvm::Type *ArgTy) {
  unsigned NumElts = ArgTy->getVectorNumElements();
  SmallVector<Constant *, 16> Indices;
  for (unsigned i = 0; i < 2 * NumElts; ++i)
    Indices.push_back(ConstantInt::get(Int32Ty, i));

  Constant *Mask = ConstantVector::get(Indices);
  Value *LoCast = Builder.CreateBitCast(Lo, ArgTy);
  Value *HiCast = Builder.CreateBitCast(Hi, ArgTy);
  return Builder.CreateShuffleVector(LoCast, HiCast, Mask, "concat");
}

Value *CodeGenFunction::EmitExtractHigh(Value *Vec, llvm::Type *ResTy) {
  unsigned NumElts = ResTy->getVectorNumElements();
  SmallVector<Constant *, 8> Indices;

  llvm::Type *InTy = llvm::VectorType::get(ResTy->getVectorElementType(),
                                           NumElts * 2);
  Value *VecCast = Builder.CreateBitCast(Vec, InTy);

  // extract_high is a shuffle on the second half of the input indices: E.g. 4,
  // 5, 6, 7 if we're extracting <4 x i16> from <8 x i16>.
  for (unsigned i = 0; i < NumElts; ++i)
    Indices.push_back(ConstantInt::get(Int32Ty, NumElts + i));

  Constant *Mask = ConstantVector::get(Indices);
  return Builder.CreateShuffleVector(VecCast, VecCast, Mask, "concat");
}

/// GetPointeeAlignment - Given an expression with a pointer type, find the
/// alignment of the type referenced by the pointer.  Skip over implicit
/// casts.
std::pair<llvm::Value*, unsigned>
CodeGenFunction::EmitPointerWithAlignment(const Expr *Addr) {
  assert(Addr->getType()->isPointerType());
  Addr = Addr->IgnoreParens();
  if (const ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(Addr)) {
    if ((ICE->getCastKind() == CK_BitCast || ICE->getCastKind() == CK_NoOp) &&
        ICE->getSubExpr()->getType()->isPointerType()) {
      std::pair<llvm::Value*, unsigned> Ptr =
          EmitPointerWithAlignment(ICE->getSubExpr());
      Ptr.first = Builder.CreateBitCast(Ptr.first,
                                        ConvertType(Addr->getType()));
      return Ptr;
    } else if (ICE->getCastKind() == CK_ArrayToPointerDecay) {
      LValue LV = EmitLValue(ICE->getSubExpr());
      unsigned Align = LV.getAlignment().getQuantity();
      if (!Align) {
        // FIXME: Once LValues are fixed to always set alignment,
        // zap this code.
        QualType PtTy = ICE->getSubExpr()->getType();
        if (!PtTy->isIncompleteType())
          Align = getContext().getTypeAlignInChars(PtTy).getQuantity();
        else
          Align = 1;
      }
      return std::make_pair(LV.getAddress(), Align);
    }
  }
  if (const UnaryOperator *UO = dyn_cast<UnaryOperator>(Addr)) {
    if (UO->getOpcode() == UO_AddrOf) {
      LValue LV = EmitLValue(UO->getSubExpr());
      unsigned Align = LV.getAlignment().getQuantity();
      if (!Align) {
        // FIXME: Once LValues are fixed to always set alignment,
        // zap this code.
        QualType PtTy = UO->getSubExpr()->getType();
        if (!PtTy->isIncompleteType())
          Align = getContext().getTypeAlignInChars(PtTy).getQuantity();
        else
          Align = 1;
      }
      return std::make_pair(LV.getAddress(), Align);
    }
  }

  unsigned Align = 1;
  QualType PtTy = Addr->getType()->getPointeeType();
  if (!PtTy->isIncompleteType())
    Align = getContext().getTypeAlignInChars(PtTy).getQuantity();

  return std::make_pair(EmitScalarExpr(Addr), Align);
}

Value *CodeGenFunction::EmitCommonNeonBuiltinExpr(
    unsigned BuiltinID, unsigned LLVMIntrinsic, unsigned AltLLVMIntrinsic,
    const CallExpr *E, SmallVectorImpl<Value *> &Ops, llvm::Value *Align) {
  // Get the last argument, which specifies the vector type.
  llvm::APSInt Result;
  const Expr *Arg = E->getArg(E->getNumArgs() - 1);
  if (!Arg->isIntegerConstantExpr(Result, getContext()))
    return 0;

  // Determine the type of this overloaded NEON intrinsic.
  NeonTypeFlags Type(Result.getZExtValue());
  bool Usgn = Type.isUnsigned();
  bool Quad = Type.isQuad();

  llvm::VectorType *VTy = GetNeonType(this, Type);
  llvm::Type *Ty = VTy;
  if (!Ty)
    return 0;

  unsigned Int;
  switch (BuiltinID) {
  default: break;
  case NEON::BI__builtin_neon_vaeseq_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic), Ops, "aese");
  case NEON::BI__builtin_neon_vaesdq_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic), Ops, "aesd");
  case NEON::BI__builtin_neon_vaesmcq_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic), Ops, "aesmc");
  case NEON::BI__builtin_neon_vaesimcq_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic), Ops, "aesimc");
  case NEON::BI__builtin_neon_vabd_v:
  case NEON::BI__builtin_neon_vabdq_v:
    Int = Usgn ? Intrinsic::arm_neon_vabdu : Intrinsic::arm_neon_vabds;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vabd");
  case NEON::BI__builtin_neon_vaddhn_v: {
    llvm::VectorType *SrcTy =
        llvm::VectorType::getExtendedElementVectorType(VTy);

    // %sum = add <4 x i32> %lhs, %rhs
    Ops[0] = Builder.CreateBitCast(Ops[0], SrcTy);
    Ops[1] = Builder.CreateBitCast(Ops[1], SrcTy);
    Ops[0] = Builder.CreateAdd(Ops[0], Ops[1], "vaddhn");

    // %high = lshr <4 x i32> %sum, <i32 16, i32 16, i32 16, i32 16>
    Constant *ShiftAmt = ConstantInt::get(SrcTy->getElementType(),
                                       SrcTy->getScalarSizeInBits() / 2);
    ShiftAmt = ConstantVector::getSplat(VTy->getNumElements(), ShiftAmt);
    Ops[0] = Builder.CreateLShr(Ops[0], ShiftAmt, "vaddhn");

    // %res = trunc <4 x i32> %high to <4 x i16>
    return Builder.CreateTrunc(Ops[0], VTy, "vaddhn");
  }
  case NEON::BI__builtin_neon_vbsl_v:
  case NEON::BI__builtin_neon_vbslq_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vbsl, Ty),
                        Ops, "vbsl");
  case NEON::BI__builtin_neon_vcale_v:
  case NEON::BI__builtin_neon_vcaleq_v:
    std::swap(Ops[0], Ops[1]);
  case NEON::BI__builtin_neon_vcage_v:
  case NEON::BI__builtin_neon_vcageq_v: {
    llvm::Type *VecFlt = llvm::VectorType::get(
        VTy->getScalarSizeInBits() == 32 ? FloatTy : DoubleTy,
        VTy->getNumElements());
    llvm::Type *Tys[] = { VTy, VecFlt };
    Function *F = CGM.getIntrinsic(LLVMIntrinsic, Tys);
    return EmitNeonCall(F, Ops, "vcage");
  }
  case NEON::BI__builtin_neon_vcalt_v:
  case NEON::BI__builtin_neon_vcaltq_v:
    std::swap(Ops[0], Ops[1]);
  case NEON::BI__builtin_neon_vcagt_v:
  case NEON::BI__builtin_neon_vcagtq_v: {
    llvm::Type *VecFlt = llvm::VectorType::get(
        VTy->getScalarSizeInBits() == 32 ? FloatTy : DoubleTy,
        VTy->getNumElements());
    llvm::Type *Tys[] = { VTy, VecFlt };
    Function *F = CGM.getIntrinsic(LLVMIntrinsic, Tys);
    return EmitNeonCall(F, Ops, "vcagt");
  }
  case NEON::BI__builtin_neon_vcls_v:
  case NEON::BI__builtin_neon_vclsq_v: {
    Function *F = CGM.getIntrinsic(Intrinsic::arm_neon_vcls, Ty);
    return EmitNeonCall(F, Ops, "vcls");
  }
  case NEON::BI__builtin_neon_vclz_v:
  case NEON::BI__builtin_neon_vclzq_v: {
    // Generate target-independent intrinsic; also need to add second argument
    // for whether or not clz of zero is undefined; on ARM it isn't.
    Function *F = CGM.getIntrinsic(Intrinsic::ctlz, Ty);
    Ops.push_back(Builder.getInt1(getTarget().isCLZForZeroUndef()));
    return EmitNeonCall(F, Ops, "vclz");
  }
  case NEON::BI__builtin_neon_vcnt_v:
  case NEON::BI__builtin_neon_vcntq_v: {
    // generate target-independent intrinsic
    Function *F = CGM.getIntrinsic(Intrinsic::ctpop, Ty);
    return EmitNeonCall(F, Ops, "vctpop");
  }
  case NEON::BI__builtin_neon_vcvt_f16_v: {
    assert(Type.getEltType() == NeonTypeFlags::Float16 && !Quad &&
           "unexpected vcvt_f16_v builtin");
    Function *F = CGM.getIntrinsic(Intrinsic::arm_neon_vcvtfp2hf);
    return EmitNeonCall(F, Ops, "vcvt");
  }
  case NEON::BI__builtin_neon_vcvt_f32_f16: {
    assert(Type.getEltType() == NeonTypeFlags::Float16 && !Quad &&
           "unexpected vcvt_f32_f16 builtin");
    Function *F = CGM.getIntrinsic(Intrinsic::arm_neon_vcvthf2fp);
    return EmitNeonCall(F, Ops, "vcvt");
  }
  case NEON::BI__builtin_neon_vcvt_f32_v:
  case NEON::BI__builtin_neon_vcvtq_f32_v:
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ty = GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float32, false, Quad));
    return Usgn ? Builder.CreateUIToFP(Ops[0], Ty, "vcvt")
                : Builder.CreateSIToFP(Ops[0], Ty, "vcvt");
  case NEON::BI__builtin_neon_vcvt_n_f32_v:
  case NEON::BI__builtin_neon_vcvtq_n_f32_v: {
    llvm::Type *FloatTy =
      GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float32, false, Quad));
    llvm::Type *Tys[2] = { FloatTy, Ty };
    Int = Usgn ? Intrinsic::arm_neon_vcvtfxu2fp
               : Intrinsic::arm_neon_vcvtfxs2fp;
    Function *F = CGM.getIntrinsic(Int, Tys);
    return EmitNeonCall(F, Ops, "vcvt_n");
  }
  case NEON::BI__builtin_neon_vcvt_n_s32_v:
  case NEON::BI__builtin_neon_vcvt_n_u32_v:
  case NEON::BI__builtin_neon_vcvtq_n_s32_v:
  case NEON::BI__builtin_neon_vcvtq_n_u32_v: {
    llvm::Type *FloatTy =
      GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float32, false, Quad));
    llvm::Type *Tys[2] = { Ty, FloatTy };
    Int = Usgn ? Intrinsic::arm_neon_vcvtfp2fxu
               : Intrinsic::arm_neon_vcvtfp2fxs;
    Function *F = CGM.getIntrinsic(Int, Tys);
    return EmitNeonCall(F, Ops, "vcvt_n");
  }
  case NEON::BI__builtin_neon_vcvt_s32_v:
  case NEON::BI__builtin_neon_vcvt_u32_v:
  case NEON::BI__builtin_neon_vcvtq_s32_v:
  case NEON::BI__builtin_neon_vcvtq_u32_v: {
    llvm::Type *FloatTy =
      GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float32, false, Quad));
    Ops[0] = Builder.CreateBitCast(Ops[0], FloatTy);
    return Usgn ? Builder.CreateFPToUI(Ops[0], Ty, "vcvt")
                : Builder.CreateFPToSI(Ops[0], Ty, "vcvt");
  }
  case NEON::BI__builtin_neon_vext_v:
  case NEON::BI__builtin_neon_vextq_v: {
    int CV = cast<ConstantInt>(Ops[2])->getSExtValue();
    SmallVector<Constant*, 16> Indices;
    for (unsigned i = 0, e = VTy->getNumElements(); i != e; ++i)
      Indices.push_back(ConstantInt::get(Int32Ty, i+CV));

    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Value *SV = llvm::ConstantVector::get(Indices);
    return Builder.CreateShuffleVector(Ops[0], Ops[1], SV, "vext");
  }
  case NEON::BI__builtin_neon_vfma_v:
  case NEON::BI__builtin_neon_vfmaq_v: {
    Value *F = CGM.getIntrinsic(Intrinsic::fma, Ty);
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);

    // NEON intrinsic puts accumulator first, unlike the LLVM fma.
    return Builder.CreateCall3(F, Ops[1], Ops[2], Ops[0]);
  }
  case NEON::BI__builtin_neon_vhadd_v:
  case NEON::BI__builtin_neon_vhaddq_v:
    Int = Usgn ? LLVMIntrinsic : AltLLVMIntrinsic;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vhadd");
  case NEON::BI__builtin_neon_vhsub_v:
  case NEON::BI__builtin_neon_vhsubq_v:
    Int = Usgn ? LLVMIntrinsic : AltLLVMIntrinsic;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vhsub");
  case NEON::BI__builtin_neon_vld1_v:
  case NEON::BI__builtin_neon_vld1q_v:
    Ops.push_back(Align);
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vld1, Ty),
                        Ops, "vld1");
  case NEON::BI__builtin_neon_vld2_v:
  case NEON::BI__builtin_neon_vld2q_v: {
    Function *F = CGM.getIntrinsic(Intrinsic::arm_neon_vld2, Ty);
    Ops[1] = Builder.CreateCall2(F, Ops[1], Align, "vld2");
    Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vld3_v:
  case NEON::BI__builtin_neon_vld3q_v: {
    Function *F = CGM.getIntrinsic(Intrinsic::arm_neon_vld3, Ty);
    Ops[1] = Builder.CreateCall2(F, Ops[1], Align, "vld3");
    Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vld4_v:
  case NEON::BI__builtin_neon_vld4q_v: {
    Function *F = CGM.getIntrinsic(Intrinsic::arm_neon_vld4, Ty);
    Ops[1] = Builder.CreateCall2(F, Ops[1], Align, "vld4");
    Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vld1_dup_v:
  case NEON::BI__builtin_neon_vld1q_dup_v: {
    Value *V = UndefValue::get(Ty);
    Ty = llvm::PointerType::getUnqual(VTy->getElementType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    LoadInst *Ld = Builder.CreateLoad(Ops[0]);
    Ld->setAlignment(cast<ConstantInt>(Align)->getZExtValue());
    llvm::Constant *CI = ConstantInt::get(Int32Ty, 0);
    Ops[0] = Builder.CreateInsertElement(V, Ld, CI);
    return EmitNeonSplat(Ops[0], CI);
  }
  case NEON::BI__builtin_neon_vld2_lane_v:
  case NEON::BI__builtin_neon_vld2q_lane_v: {
    Function *F = CGM.getIntrinsic(Intrinsic::arm_neon_vld2lane, Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);
    Ops[3] = Builder.CreateBitCast(Ops[3], Ty);
    Ops.push_back(Align);
    Ops[1] = Builder.CreateCall(F, makeArrayRef(Ops).slice(1), "vld2_lane");
    Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vld3_lane_v:
  case NEON::BI__builtin_neon_vld3q_lane_v: {
    Function *F = CGM.getIntrinsic(Intrinsic::arm_neon_vld3lane, Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);
    Ops[3] = Builder.CreateBitCast(Ops[3], Ty);
    Ops[4] = Builder.CreateBitCast(Ops[4], Ty);
    Ops.push_back(Align);
    Ops[1] = Builder.CreateCall(F, makeArrayRef(Ops).slice(1), "vld3_lane");
    Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vld4_lane_v:
  case NEON::BI__builtin_neon_vld4q_lane_v: {
    Function *F = CGM.getIntrinsic(Intrinsic::arm_neon_vld4lane, Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);
    Ops[3] = Builder.CreateBitCast(Ops[3], Ty);
    Ops[4] = Builder.CreateBitCast(Ops[4], Ty);
    Ops[5] = Builder.CreateBitCast(Ops[5], Ty);
    Ops.push_back(Align);
    Ops[1] = Builder.CreateCall(F, makeArrayRef(Ops).slice(1), "vld3_lane");
    Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vmax_v:
  case NEON::BI__builtin_neon_vmaxq_v:
    Int = Usgn ? Intrinsic::arm_neon_vmaxu : Intrinsic::arm_neon_vmaxs;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vmax");
  case NEON::BI__builtin_neon_vmin_v:
  case NEON::BI__builtin_neon_vminq_v:
    Int = Usgn ? Intrinsic::arm_neon_vminu : Intrinsic::arm_neon_vmins;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vmin");
  case NEON::BI__builtin_neon_vmovl_v: {
    llvm::Type *DTy =llvm::VectorType::getTruncatedElementVectorType(VTy);
    Ops[0] = Builder.CreateBitCast(Ops[0], DTy);
    if (Usgn)
      return Builder.CreateZExt(Ops[0], Ty, "vmovl");
    return Builder.CreateSExt(Ops[0], Ty, "vmovl");
  }
  case NEON::BI__builtin_neon_vmovn_v: {
    llvm::Type *QTy = llvm::VectorType::getExtendedElementVectorType(VTy);
    Ops[0] = Builder.CreateBitCast(Ops[0], QTy);
    return Builder.CreateTrunc(Ops[0], Ty, "vmovn");
  }
  case NEON::BI__builtin_neon_vmul_v:
  case NEON::BI__builtin_neon_vmulq_v:
    assert(Type.isPoly() && "vmul builtin only supported for polynomial types");
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic, Ty), Ops, "vmul");
  case NEON::BI__builtin_neon_vmull_v:
    // FIXME: the integer vmull operations could be emitted in terms of pure
    // LLVM IR (2 exts followed by a mul). Unfortunately LLVM has a habit of
    // hoisting the exts outside loops. Until global ISel comes along that can
    // see through such movement this leads to bad CodeGen. So we need an
    // intrinsic for now.
    Int = Usgn ? Intrinsic::arm_neon_vmullu : Intrinsic::arm_neon_vmulls;
    Int = Type.isPoly() ? (unsigned)Intrinsic::arm_neon_vmullp : Int;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vmull");
  case NEON::BI__builtin_neon_vpadal_v:
  case NEON::BI__builtin_neon_vpadalq_v: {
    Int = Usgn ? Intrinsic::arm_neon_vpadalu : Intrinsic::arm_neon_vpadals;
    // The source operand type has twice as many elements of half the size.
    unsigned EltBits = VTy->getElementType()->getPrimitiveSizeInBits();
    llvm::Type *EltTy =
      llvm::IntegerType::get(getLLVMContext(), EltBits / 2);
    llvm::Type *NarrowTy =
      llvm::VectorType::get(EltTy, VTy->getNumElements() * 2);
    llvm::Type *Tys[2] = { Ty, NarrowTy };
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vpadal");
  }
  case NEON::BI__builtin_neon_vpadd_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic, Ty), Ops, "vpadd");
  case NEON::BI__builtin_neon_vpaddl_v:
  case NEON::BI__builtin_neon_vpaddlq_v: {
    Int = Usgn ? LLVMIntrinsic : AltLLVMIntrinsic;
    // The source operand type has twice as many elements of half the size.
    unsigned EltBits = VTy->getElementType()->getPrimitiveSizeInBits();
    llvm::Type *EltTy = llvm::IntegerType::get(getLLVMContext(), EltBits / 2);
    llvm::Type *NarrowTy =
      llvm::VectorType::get(EltTy, VTy->getNumElements() * 2);
    llvm::Type *Tys[2] = { Ty, NarrowTy };
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vpaddl");
  }
  case NEON::BI__builtin_neon_vpmax_v:
    Int = Usgn ? Intrinsic::arm_neon_vpmaxu : Intrinsic::arm_neon_vpmaxs;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vpmax");
  case NEON::BI__builtin_neon_vpmin_v:
    Int = Usgn ? Intrinsic::arm_neon_vpminu : Intrinsic::arm_neon_vpmins;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vpmin");
  case NEON::BI__builtin_neon_vqabs_v:
  case NEON::BI__builtin_neon_vqabsq_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vqabs, Ty),
                        Ops, "vqabs");
  case NEON::BI__builtin_neon_vqadd_v:
  case NEON::BI__builtin_neon_vqaddq_v:
    Int = Usgn ? LLVMIntrinsic : AltLLVMIntrinsic;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqadd");
  case NEON::BI__builtin_neon_vqmovn_v:
    Int = Usgn ? Intrinsic::arm_neon_vqmovnu : Intrinsic::arm_neon_vqmovns;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqmovn");
  case NEON::BI__builtin_neon_vqmovun_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vqmovnsu, Ty),
                        Ops, "vqdmull");
  case NEON::BI__builtin_neon_vqneg_v:
  case NEON::BI__builtin_neon_vqnegq_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vqneg, Ty),
                        Ops, "vqneg");
  case NEON::BI__builtin_neon_vqsub_v:
  case NEON::BI__builtin_neon_vqsubq_v:
    Int = Usgn ? LLVMIntrinsic : AltLLVMIntrinsic;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqsub");
  case NEON::BI__builtin_neon_vqdmlal_v: {
    SmallVector<Value *, 2> MulOps(Ops.begin() + 1, Ops.end());
    Value *Mul = EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vqdmull, Ty),
                              MulOps, "vqdmlal");

    SmallVector<Value *, 2> AddOps;
    AddOps.push_back(Ops[0]);
    AddOps.push_back(Mul);
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vqadds, Ty),
                        AddOps, "vqdmlal");
  }
  case NEON::BI__builtin_neon_vqdmlsl_v: {
    SmallVector<Value *, 2> MulOps(Ops.begin() + 1, Ops.end());
    Value *Mul = EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vqdmull, Ty),
                              MulOps, "vqdmlsl");

    SmallVector<Value *, 2> SubOps;
    SubOps.push_back(Ops[0]);
    SubOps.push_back(Mul);
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vqsubs, Ty),
                        SubOps, "vqdmlsl");
  }
  case NEON::BI__builtin_neon_vqdmulh_v:
  case NEON::BI__builtin_neon_vqdmulhq_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic, Ty), Ops, "vqdmulh");
  case NEON::BI__builtin_neon_vqdmull_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic, Ty), Ops, "vqdmull");
  case NEON::BI__builtin_neon_vqshl_n_v:
  case NEON::BI__builtin_neon_vqshlq_n_v:
    Int = Usgn ? LLVMIntrinsic : AltLLVMIntrinsic;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqshl_n",
                        1, false);
  case NEON::BI__builtin_neon_vqrdmulh_v:
  case NEON::BI__builtin_neon_vqrdmulhq_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic, Ty), Ops, "vqrdmulh");
  case NEON::BI__builtin_neon_vqrshl_v:
  case NEON::BI__builtin_neon_vqrshlq_v:
    Int = Usgn ? LLVMIntrinsic : AltLLVMIntrinsic;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqrshl");
  case NEON::BI__builtin_neon_vqshl_v:
  case NEON::BI__builtin_neon_vqshlq_v:
    Int = Usgn ? LLVMIntrinsic : AltLLVMIntrinsic;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqshl");
  case NEON::BI__builtin_neon_vraddhn_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic, Ty),
                        Ops, "vraddhn");
  case NEON::BI__builtin_neon_vrecpe_v:
  case NEON::BI__builtin_neon_vrecpeq_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vrecpe, Ty),
                        Ops, "vrecpe");
  case NEON::BI__builtin_neon_vrecps_v:
  case NEON::BI__builtin_neon_vrecpsq_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic, Ty), Ops, "vrecps");
  case NEON::BI__builtin_neon_vrhadd_v:
  case NEON::BI__builtin_neon_vrhaddq_v:
    Int = Usgn ? LLVMIntrinsic: AltLLVMIntrinsic;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrhadd");
  case NEON::BI__builtin_neon_vrshl_v:
  case NEON::BI__builtin_neon_vrshlq_v:
    Int = Usgn ? LLVMIntrinsic : AltLLVMIntrinsic;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrshl");
  case NEON::BI__builtin_neon_vrsqrte_v:
  case NEON::BI__builtin_neon_vrsqrteq_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vrsqrte, Ty),
                        Ops, "vrsqrte");
  case NEON::BI__builtin_neon_vrsqrts_v:
  case NEON::BI__builtin_neon_vrsqrtsq_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vrsqrts, Ty),
                        Ops, "vrsqrts");
  case NEON::BI__builtin_neon_vrsubhn_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic, Ty), Ops, "vrsubhn");

  case NEON::BI__builtin_neon_vsha1su1q_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic), Ops, "sha1su1");
  case NEON::BI__builtin_neon_vsha256su0q_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic), Ops, "sha256su0");
  case NEON::BI__builtin_neon_vsha1su0q_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic), Ops, "sha1su0");
  case NEON::BI__builtin_neon_vsha256hq_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic), Ops, "sha256h");
  case NEON::BI__builtin_neon_vsha256h2q_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic), Ops, "sha256h2");
  case NEON::BI__builtin_neon_vsha256su1q_v:
    return EmitNeonCall(CGM.getIntrinsic(LLVMIntrinsic), Ops, "sha256su1");

  case NEON::BI__builtin_neon_vshl_n_v:
  case NEON::BI__builtin_neon_vshlq_n_v:
    Ops[1] = EmitNeonShiftVector(Ops[1], Ty, false);
    return Builder.CreateShl(Builder.CreateBitCast(Ops[0],Ty), Ops[1],
                             "vshl_n");
  case NEON::BI__builtin_neon_vshl_v:
  case NEON::BI__builtin_neon_vshlq_v:
    Int = Usgn ? LLVMIntrinsic : AltLLVMIntrinsic;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vshl");
  case NEON::BI__builtin_neon_vshrn_n_v: {
    llvm::Type *SrcTy = llvm::VectorType::getExtendedElementVectorType(VTy);
    Ops[0] = Builder.CreateBitCast(Ops[0], SrcTy);
    Ops[1] = EmitNeonShiftVector(Ops[1], SrcTy, false);
    if (Usgn)
      Ops[0] = Builder.CreateLShr(Ops[0], Ops[1]);
    else
      Ops[0] = Builder.CreateAShr(Ops[0], Ops[1]);
    return Builder.CreateTrunc(Ops[0], Ty, "vshrn_n");
  }
  case NEON::BI__builtin_neon_vshr_n_v:
  case NEON::BI__builtin_neon_vshrq_n_v:
    return EmitNeonRShiftImm(Ops[0], Ops[1], Ty, Usgn, "vshr_n");
  case NEON::BI__builtin_neon_vst1_v:
  case NEON::BI__builtin_neon_vst1q_v:
    Ops.push_back(Align);
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vst1, Ty),
                        Ops, "");
  case NEON::BI__builtin_neon_vst2_v:
  case NEON::BI__builtin_neon_vst2q_v:
    Ops.push_back(Align);
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vst2, Ty),
                        Ops, "");
  case NEON::BI__builtin_neon_vst3_v:
  case NEON::BI__builtin_neon_vst3q_v:
    Ops.push_back(Align);
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vst3, Ty),
                        Ops, "");
  case NEON::BI__builtin_neon_vst4_v:
  case NEON::BI__builtin_neon_vst4q_v:
    Ops.push_back(Align);
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vst4, Ty),
                        Ops, "");
  case NEON::BI__builtin_neon_vst2_lane_v:
  case NEON::BI__builtin_neon_vst2q_lane_v:
    Ops.push_back(Align);
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vst2lane, Ty),
                        Ops, "");
  case NEON::BI__builtin_neon_vst3_lane_v:
  case NEON::BI__builtin_neon_vst3q_lane_v:
    Ops.push_back(Align);
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vst3lane, Ty),
                        Ops, "");
  case NEON::BI__builtin_neon_vst4_lane_v:
  case NEON::BI__builtin_neon_vst4q_lane_v:
    Ops.push_back(Align);
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vst4lane, Ty),
                        Ops, "");
  case NEON::BI__builtin_neon_vsubhn_v: {
    llvm::VectorType *SrcTy =
        llvm::VectorType::getExtendedElementVectorType(VTy);

    // %sum = add <4 x i32> %lhs, %rhs
    Ops[0] = Builder.CreateBitCast(Ops[0], SrcTy);
    Ops[1] = Builder.CreateBitCast(Ops[1], SrcTy);
    Ops[0] = Builder.CreateSub(Ops[0], Ops[1], "vsubhn");

    // %high = lshr <4 x i32> %sum, <i32 16, i32 16, i32 16, i32 16>
    Constant *ShiftAmt = ConstantInt::get(SrcTy->getElementType(),
                                       SrcTy->getScalarSizeInBits() / 2);
    ShiftAmt = ConstantVector::getSplat(VTy->getNumElements(), ShiftAmt);
    Ops[0] = Builder.CreateLShr(Ops[0], ShiftAmt, "vsubhn");

    // %res = trunc <4 x i32> %high to <4 x i16>
    return Builder.CreateTrunc(Ops[0], VTy, "vsubhn");
  }
  case NEON::BI__builtin_neon_vtrn_v:
  case NEON::BI__builtin_neon_vtrnq_v: {
    Ops[0] = Builder.CreateBitCast(Ops[0], llvm::PointerType::getUnqual(Ty));
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);
    Value *SV = 0;

    for (unsigned vi = 0; vi != 2; ++vi) {
      SmallVector<Constant*, 16> Indices;
      for (unsigned i = 0, e = VTy->getNumElements(); i != e; i += 2) {
        Indices.push_back(Builder.getInt32(i+vi));
        Indices.push_back(Builder.getInt32(i+e+vi));
      }
      Value *Addr = Builder.CreateConstInBoundsGEP1_32(Ops[0], vi);
      SV = llvm::ConstantVector::get(Indices);
      SV = Builder.CreateShuffleVector(Ops[1], Ops[2], SV, "vtrn");
      SV = Builder.CreateStore(SV, Addr);
    }
    return SV;
  }
  case NEON::BI__builtin_neon_vtst_v:
  case NEON::BI__builtin_neon_vtstq_v: {
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[0] = Builder.CreateAnd(Ops[0], Ops[1]);
    Ops[0] = Builder.CreateICmp(ICmpInst::ICMP_NE, Ops[0],
                                ConstantAggregateZero::get(Ty));
    return Builder.CreateSExt(Ops[0], Ty, "vtst");
  }
  case NEON::BI__builtin_neon_vuzp_v:
  case NEON::BI__builtin_neon_vuzpq_v: {
    Ops[0] = Builder.CreateBitCast(Ops[0], llvm::PointerType::getUnqual(Ty));
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);
    Value *SV = 0;

    for (unsigned vi = 0; vi != 2; ++vi) {
      SmallVector<Constant*, 16> Indices;
      for (unsigned i = 0, e = VTy->getNumElements(); i != e; ++i)
        Indices.push_back(ConstantInt::get(Int32Ty, 2*i+vi));

      Value *Addr = Builder.CreateConstInBoundsGEP1_32(Ops[0], vi);
      SV = llvm::ConstantVector::get(Indices);
      SV = Builder.CreateShuffleVector(Ops[1], Ops[2], SV, "vuzp");
      SV = Builder.CreateStore(SV, Addr);
    }
    return SV;
  }
  case NEON::BI__builtin_neon_vzip_v:
  case NEON::BI__builtin_neon_vzipq_v: {
    Ops[0] = Builder.CreateBitCast(Ops[0], llvm::PointerType::getUnqual(Ty));
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);
    Value *SV = 0;

    for (unsigned vi = 0; vi != 2; ++vi) {
      SmallVector<Constant*, 16> Indices;
      for (unsigned i = 0, e = VTy->getNumElements(); i != e; i += 2) {
        Indices.push_back(ConstantInt::get(Int32Ty, (i + vi*e) >> 1));
        Indices.push_back(ConstantInt::get(Int32Ty, ((i + vi*e) >> 1)+e));
      }
      Value *Addr = Builder.CreateConstInBoundsGEP1_32(Ops[0], vi);
      SV = llvm::ConstantVector::get(Indices);
      SV = Builder.CreateShuffleVector(Ops[1], Ops[2], SV, "vzip");
      SV = Builder.CreateStore(SV, Addr);
    }
    return SV;
  }
  }

  return 0;
}

static Value *EmitAArch64ScalarBuiltinExpr(CodeGenFunction &CGF,
                                           unsigned BuiltinID,
                                           const CallExpr *E) {
  unsigned int Int = 0;
  unsigned IntTypes = 0;
  enum {
    ScalarRet = (1 << 0),
    VectorRet = (1 << 1),
    ScalarArg0 = (1 << 2),
    VectorGetArg0 = (1 << 3),
    VectorCastArg0 = (1 << 4),
    ScalarArg1 = (1 << 5),
    VectorGetArg1 = (1 << 6),
    VectorCastArg1 = (1 << 7),
    ScalarFpCmpzArg1 = (1 << 8)
  };
  const char *s = NULL;

  SmallVector<Value *, 4> Ops;
  for (unsigned i = 0, e = E->getNumArgs(); i != e; i++) {
    Ops.push_back(CGF.EmitScalarExpr(E->getArg(i)));
  }

  // AArch64 scalar builtins are not overloaded, they do not have an extra
  // argument that specifies the vector type, need to handle each case.
  switch (BuiltinID) {
  default: break;
  case NEON::BI__builtin_neon_vdups_lane_f32:
  case NEON::BI__builtin_neon_vdupd_lane_f64:
  case NEON::BI__builtin_neon_vdups_laneq_f32:
  case NEON::BI__builtin_neon_vdupd_laneq_f64: {
    return CGF.Builder.CreateExtractElement(Ops[0], Ops[1], "vdup_lane");
  }
  case NEON::BI__builtin_neon_vdupb_lane_i8:
  case NEON::BI__builtin_neon_vduph_lane_i16:
  case NEON::BI__builtin_neon_vdups_lane_i32:
  case NEON::BI__builtin_neon_vdupd_lane_i64:
  case NEON::BI__builtin_neon_vdupb_laneq_i8:
  case NEON::BI__builtin_neon_vduph_laneq_i16:
  case NEON::BI__builtin_neon_vdups_laneq_i32:
  case NEON::BI__builtin_neon_vdupd_laneq_i64: {
    // The backend treats Neon scalar types as v1ix types
    // So we want to dup lane from any vector to v1ix vector
    // with shufflevector
    s = "vdup_lane";
    Value* SV = llvm::ConstantVector::getSplat(1, cast<ConstantInt>(Ops[1]));
    Value *Result = CGF.Builder.CreateShuffleVector(Ops[0], Ops[0], SV, s);
    llvm::Type *Ty = CGF.ConvertType(E->getCallReturnType());
    // AArch64 intrinsic one-element vector type cast to
    // scalar type expected by the builtin
    return CGF.Builder.CreateBitCast(Result, Ty, s);
  }
  case NEON::BI__builtin_neon_vqdmlalh_lane_s16 :
  case NEON::BI__builtin_neon_vqdmlalh_laneq_s16 :
  case NEON::BI__builtin_neon_vqdmlals_lane_s32 :
  case NEON::BI__builtin_neon_vqdmlals_laneq_s32 :
  case NEON::BI__builtin_neon_vqdmlslh_lane_s16 :
  case NEON::BI__builtin_neon_vqdmlslh_laneq_s16 :
  case NEON::BI__builtin_neon_vqdmlsls_lane_s32 :
  case NEON::BI__builtin_neon_vqdmlsls_laneq_s32 : {
    Int = Intrinsic::arm_neon_vqadds;
    if (BuiltinID == NEON::BI__builtin_neon_vqdmlslh_lane_s16 ||
        BuiltinID == NEON::BI__builtin_neon_vqdmlslh_laneq_s16 ||
        BuiltinID == NEON::BI__builtin_neon_vqdmlsls_lane_s32 ||
        BuiltinID == NEON::BI__builtin_neon_vqdmlsls_laneq_s32) {
      Int = Intrinsic::arm_neon_vqsubs;
    }
    // create vqdmull call with b * c[i]
    llvm::Type *Ty = CGF.ConvertType(E->getArg(1)->getType());
    llvm::VectorType *OpVTy = llvm::VectorType::get(Ty, 1);
    Ty = CGF.ConvertType(E->getArg(0)->getType());
    llvm::VectorType *ResVTy = llvm::VectorType::get(Ty, 1);
    Value *F = CGF.CGM.getIntrinsic(Intrinsic::arm_neon_vqdmull, ResVTy);
    Value *V = UndefValue::get(OpVTy);
    llvm::Constant *CI = ConstantInt::get(CGF.Int32Ty, 0);
    SmallVector<Value *, 2> MulOps;
    MulOps.push_back(Ops[1]);
    MulOps.push_back(Ops[2]);
    MulOps[0] = CGF.Builder.CreateInsertElement(V, MulOps[0], CI);
    MulOps[1] = CGF.Builder.CreateExtractElement(MulOps[1], Ops[3], "extract");
    MulOps[1] = CGF.Builder.CreateInsertElement(V, MulOps[1], CI);
    Value *MulRes = CGF.Builder.CreateCall2(F, MulOps[0], MulOps[1]);
    // create vqadds call with a +/- vqdmull result
    F = CGF.CGM.getIntrinsic(Int, ResVTy);
    SmallVector<Value *, 2> AddOps;
    AddOps.push_back(Ops[0]);
    AddOps.push_back(MulRes);
    V = UndefValue::get(ResVTy);
    AddOps[0] = CGF.Builder.CreateInsertElement(V, AddOps[0], CI);
    Value *AddRes = CGF.Builder.CreateCall2(F, AddOps[0], AddOps[1]);
    return CGF.Builder.CreateBitCast(AddRes, Ty);
  }
  case NEON::BI__builtin_neon_vfmas_lane_f32:
  case NEON::BI__builtin_neon_vfmas_laneq_f32:
  case NEON::BI__builtin_neon_vfmad_lane_f64:
  case NEON::BI__builtin_neon_vfmad_laneq_f64: {
    llvm::Type *Ty = CGF.ConvertType(E->getCallReturnType());
    Value *F = CGF.CGM.getIntrinsic(Intrinsic::fma, Ty);
    Ops[2] = CGF.Builder.CreateExtractElement(Ops[2], Ops[3], "extract");
    return CGF.Builder.CreateCall3(F, Ops[1], Ops[2], Ops[0]);
  }
  // Scalar Floating-point Multiply Extended
  case NEON::BI__builtin_neon_vmulxs_f32:
  case NEON::BI__builtin_neon_vmulxd_f64: {
    Int = Intrinsic::aarch64_neon_vmulx;
    llvm::Type *Ty = CGF.ConvertType(E->getCallReturnType());
    return CGF.EmitNeonCall(CGF.CGM.getIntrinsic(Int, Ty), Ops, "vmulx");
  }
  case NEON::BI__builtin_neon_vmul_n_f64: {
    // v1f64 vmul_n_f64  should be mapped to Neon scalar mul lane
    llvm::Type *VTy = GetNeonType(&CGF,
      NeonTypeFlags(NeonTypeFlags::Float64, false, false));
    Ops[0] = CGF.Builder.CreateBitCast(Ops[0], VTy);
    llvm::Value *Idx = llvm::ConstantInt::get(CGF.Int32Ty, 0);
    Ops[0] = CGF.Builder.CreateExtractElement(Ops[0], Idx, "extract");
    Value *Result = CGF.Builder.CreateFMul(Ops[0], Ops[1]);
    return CGF.Builder.CreateBitCast(Result, VTy);
  }
  case NEON::BI__builtin_neon_vget_lane_i8:
  case NEON::BI__builtin_neon_vget_lane_i16:
  case NEON::BI__builtin_neon_vget_lane_i32:
  case NEON::BI__builtin_neon_vget_lane_i64:
  case NEON::BI__builtin_neon_vget_lane_f32:
  case NEON::BI__builtin_neon_vget_lane_f64:
  case NEON::BI__builtin_neon_vgetq_lane_i8:
  case NEON::BI__builtin_neon_vgetq_lane_i16:
  case NEON::BI__builtin_neon_vgetq_lane_i32:
  case NEON::BI__builtin_neon_vgetq_lane_i64:
  case NEON::BI__builtin_neon_vgetq_lane_f32:
  case NEON::BI__builtin_neon_vgetq_lane_f64:
    return CGF.EmitARMBuiltinExpr(NEON::BI__builtin_neon_vget_lane_i8, E);
  case NEON::BI__builtin_neon_vset_lane_i8:
  case NEON::BI__builtin_neon_vset_lane_i16:
  case NEON::BI__builtin_neon_vset_lane_i32:
  case NEON::BI__builtin_neon_vset_lane_i64:
  case NEON::BI__builtin_neon_vset_lane_f32:
  case NEON::BI__builtin_neon_vset_lane_f64:
  case NEON::BI__builtin_neon_vsetq_lane_i8:
  case NEON::BI__builtin_neon_vsetq_lane_i16:
  case NEON::BI__builtin_neon_vsetq_lane_i32:
  case NEON::BI__builtin_neon_vsetq_lane_i64:
  case NEON::BI__builtin_neon_vsetq_lane_f32:
  case NEON::BI__builtin_neon_vsetq_lane_f64:
    return CGF.EmitARMBuiltinExpr(NEON::BI__builtin_neon_vset_lane_i8, E);
  // Crypto
  case NEON::BI__builtin_neon_vsha1h_u32:
    Int = Intrinsic::arm_neon_sha1h;
    s = "sha1h"; break;
  case NEON::BI__builtin_neon_vsha1cq_u32:
    Int = Intrinsic::arm_neon_sha1c;
    s = "sha1c"; break;
  case NEON::BI__builtin_neon_vsha1pq_u32:
    Int = Intrinsic::arm_neon_sha1p;
    s = "sha1p"; break;
  case NEON::BI__builtin_neon_vsha1mq_u32:
    Int = Intrinsic::arm_neon_sha1m;
    s = "sha1m"; break;
  // Scalar Add
  case NEON::BI__builtin_neon_vaddd_s64:
    Int = Intrinsic::aarch64_neon_vaddds;
    s = "vaddds"; break;
  case NEON::BI__builtin_neon_vaddd_u64:
    Int = Intrinsic::aarch64_neon_vadddu;
    s = "vadddu"; break;
  // Scalar Sub
  case NEON::BI__builtin_neon_vsubd_s64:
    Int = Intrinsic::aarch64_neon_vsubds;
    s = "vsubds"; break;
  case NEON::BI__builtin_neon_vsubd_u64:
    Int = Intrinsic::aarch64_neon_vsubdu;
    s = "vsubdu"; break;
  // Scalar Saturating Add
  case NEON::BI__builtin_neon_vqaddb_s8:
  case NEON::BI__builtin_neon_vqaddh_s16:
  case NEON::BI__builtin_neon_vqadds_s32:
  case NEON::BI__builtin_neon_vqaddd_s64:
    Int = Intrinsic::arm_neon_vqadds;
    s = "vqadds"; IntTypes = VectorRet; break;
  case NEON::BI__builtin_neon_vqaddb_u8:
  case NEON::BI__builtin_neon_vqaddh_u16:
  case NEON::BI__builtin_neon_vqadds_u32:
  case NEON::BI__builtin_neon_vqaddd_u64:
    Int = Intrinsic::arm_neon_vqaddu;
    s = "vqaddu"; IntTypes = VectorRet; break;
  // Scalar Saturating Sub
  case NEON::BI__builtin_neon_vqsubb_s8:
  case NEON::BI__builtin_neon_vqsubh_s16:
  case NEON::BI__builtin_neon_vqsubs_s32:
  case NEON::BI__builtin_neon_vqsubd_s64:
    Int = Intrinsic::arm_neon_vqsubs;
    s = "vqsubs"; IntTypes = VectorRet; break;
  case NEON::BI__builtin_neon_vqsubb_u8:
  case NEON::BI__builtin_neon_vqsubh_u16:
  case NEON::BI__builtin_neon_vqsubs_u32:
  case NEON::BI__builtin_neon_vqsubd_u64:
    Int = Intrinsic::arm_neon_vqsubu;
    s = "vqsubu"; IntTypes = VectorRet; break;
  // Scalar Shift Left
  case NEON::BI__builtin_neon_vshld_s64:
    Int = Intrinsic::aarch64_neon_vshlds;
    s = "vshlds"; break;
  case NEON::BI__builtin_neon_vshld_u64:
    Int = Intrinsic::aarch64_neon_vshldu;
    s = "vshldu"; break;
  // Scalar Saturating Shift Left
  case NEON::BI__builtin_neon_vqshlb_s8:
  case NEON::BI__builtin_neon_vqshlh_s16:
  case NEON::BI__builtin_neon_vqshls_s32:
  case NEON::BI__builtin_neon_vqshld_s64:
    Int = Intrinsic::aarch64_neon_vqshls;
    s = "vqshls"; IntTypes = VectorRet; break;
  case NEON::BI__builtin_neon_vqshlb_u8:
  case NEON::BI__builtin_neon_vqshlh_u16:
  case NEON::BI__builtin_neon_vqshls_u32:
  case NEON::BI__builtin_neon_vqshld_u64:
    Int = Intrinsic::aarch64_neon_vqshlu;
    s = "vqshlu"; IntTypes = VectorRet; break;
  // Scalar Rouding Shift Left
  case NEON::BI__builtin_neon_vrshld_s64:
    Int = Intrinsic::aarch64_neon_vrshlds;
    s = "vrshlds"; break;
  case NEON::BI__builtin_neon_vrshld_u64:
    Int = Intrinsic::aarch64_neon_vrshldu;
    s = "vrshldu"; break;
  // Scalar Saturating Rouding Shift Left
  case NEON::BI__builtin_neon_vqrshlb_s8:
  case NEON::BI__builtin_neon_vqrshlh_s16:
  case NEON::BI__builtin_neon_vqrshls_s32:
  case NEON::BI__builtin_neon_vqrshld_s64:
    Int = Intrinsic::aarch64_neon_vqrshls;
    s = "vqrshls"; IntTypes = VectorRet; break;
  case NEON::BI__builtin_neon_vqrshlb_u8:
  case NEON::BI__builtin_neon_vqrshlh_u16:
  case NEON::BI__builtin_neon_vqrshls_u32:
  case NEON::BI__builtin_neon_vqrshld_u64:
    Int = Intrinsic::aarch64_neon_vqrshlu;
    s = "vqrshlu"; IntTypes = VectorRet; break;
  // Scalar Reduce Pairwise Add
  case NEON::BI__builtin_neon_vpaddd_s64:
  case NEON::BI__builtin_neon_vpaddd_u64:
    Int = Intrinsic::aarch64_neon_vpadd;
    s = "vpadd"; break;
  case NEON::BI__builtin_neon_vaddv_f32:
  case NEON::BI__builtin_neon_vaddvq_f32:
  case NEON::BI__builtin_neon_vaddvq_f64:
  case NEON::BI__builtin_neon_vpadds_f32:
  case NEON::BI__builtin_neon_vpaddd_f64:
    Int = Intrinsic::aarch64_neon_vpfadd;
    s = "vpfadd"; IntTypes = ScalarRet | VectorCastArg0; break;
  // Scalar Reduce Pairwise Floating Point Max
  case NEON::BI__builtin_neon_vmaxv_f32:
  case NEON::BI__builtin_neon_vpmaxs_f32:
  case NEON::BI__builtin_neon_vmaxvq_f64:
  case NEON::BI__builtin_neon_vpmaxqd_f64:
    Int = Intrinsic::aarch64_neon_vpmax;
    s = "vpmax"; IntTypes = ScalarRet | VectorCastArg0; break;
  // Scalar Reduce Pairwise Floating Point Min
  case NEON::BI__builtin_neon_vminv_f32:
  case NEON::BI__builtin_neon_vpmins_f32:
  case NEON::BI__builtin_neon_vminvq_f64:
  case NEON::BI__builtin_neon_vpminqd_f64:
    Int = Intrinsic::aarch64_neon_vpmin;
    s = "vpmin"; IntTypes = ScalarRet | VectorCastArg0; break;
  // Scalar Reduce Pairwise Floating Point Maxnm
  case NEON::BI__builtin_neon_vmaxnmv_f32:
  case NEON::BI__builtin_neon_vpmaxnms_f32:
  case NEON::BI__builtin_neon_vmaxnmvq_f64:
  case NEON::BI__builtin_neon_vpmaxnmqd_f64:
    Int = Intrinsic::aarch64_neon_vpfmaxnm;
    s = "vpfmaxnm"; IntTypes = ScalarRet | VectorCastArg0; break;
  // Scalar Reduce Pairwise Floating Point Minnm
  case NEON::BI__builtin_neon_vminnmv_f32:
  case NEON::BI__builtin_neon_vpminnms_f32:
  case NEON::BI__builtin_neon_vminnmvq_f64:
  case NEON::BI__builtin_neon_vpminnmqd_f64:
    Int = Intrinsic::aarch64_neon_vpfminnm;
    s = "vpfminnm"; IntTypes = ScalarRet | VectorCastArg0; break;
  // The followings are intrinsics with scalar results generated AcrossVec vectors
  case NEON::BI__builtin_neon_vaddlv_s8:
  case NEON::BI__builtin_neon_vaddlv_s16:
  case NEON::BI__builtin_neon_vaddlv_s32:
  case NEON::BI__builtin_neon_vaddlvq_s8:
  case NEON::BI__builtin_neon_vaddlvq_s16:
  case NEON::BI__builtin_neon_vaddlvq_s32:
    Int = Intrinsic::aarch64_neon_saddlv;
    s = "saddlv"; IntTypes = VectorRet | VectorCastArg1; break;
  case NEON::BI__builtin_neon_vaddlv_u8:
  case NEON::BI__builtin_neon_vaddlv_u16:
  case NEON::BI__builtin_neon_vaddlv_u32:
  case NEON::BI__builtin_neon_vaddlvq_u8:
  case NEON::BI__builtin_neon_vaddlvq_u16:
  case NEON::BI__builtin_neon_vaddlvq_u32:
    Int = Intrinsic::aarch64_neon_uaddlv;
    s = "uaddlv"; IntTypes = VectorRet | VectorCastArg1; break;
  case NEON::BI__builtin_neon_vmaxv_s8:
  case NEON::BI__builtin_neon_vmaxv_s16:
  case NEON::BI__builtin_neon_vmaxv_s32:
  case NEON::BI__builtin_neon_vmaxvq_s8:
  case NEON::BI__builtin_neon_vmaxvq_s16:
  case NEON::BI__builtin_neon_vmaxvq_s32:
    Int = Intrinsic::aarch64_neon_smaxv;
    s = "smaxv"; IntTypes = VectorRet | VectorCastArg1; break;
  case NEON::BI__builtin_neon_vmaxv_u8:
  case NEON::BI__builtin_neon_vmaxv_u16:
  case NEON::BI__builtin_neon_vmaxv_u32:
  case NEON::BI__builtin_neon_vmaxvq_u8:
  case NEON::BI__builtin_neon_vmaxvq_u16:
  case NEON::BI__builtin_neon_vmaxvq_u32:
    Int = Intrinsic::aarch64_neon_umaxv;
    s = "umaxv"; IntTypes = VectorRet | VectorCastArg1; break;
  case NEON::BI__builtin_neon_vminv_s8:
  case NEON::BI__builtin_neon_vminv_s16:
  case NEON::BI__builtin_neon_vminv_s32:
  case NEON::BI__builtin_neon_vminvq_s8:
  case NEON::BI__builtin_neon_vminvq_s16:
  case NEON::BI__builtin_neon_vminvq_s32:
    Int = Intrinsic::aarch64_neon_sminv;
    s = "sminv"; IntTypes = VectorRet | VectorCastArg1; break;
  case NEON::BI__builtin_neon_vminv_u8:
  case NEON::BI__builtin_neon_vminv_u16:
  case NEON::BI__builtin_neon_vminv_u32:
  case NEON::BI__builtin_neon_vminvq_u8:
  case NEON::BI__builtin_neon_vminvq_u16:
  case NEON::BI__builtin_neon_vminvq_u32:
    Int = Intrinsic::aarch64_neon_uminv;
    s = "uminv"; IntTypes = VectorRet | VectorCastArg1; break;
  case NEON::BI__builtin_neon_vaddv_s8:
  case NEON::BI__builtin_neon_vaddv_s16:
  case NEON::BI__builtin_neon_vaddv_s32:
  case NEON::BI__builtin_neon_vaddvq_s8:
  case NEON::BI__builtin_neon_vaddvq_s16:
  case NEON::BI__builtin_neon_vaddvq_s32:
  case NEON::BI__builtin_neon_vaddvq_s64:
  case NEON::BI__builtin_neon_vaddv_u8:
  case NEON::BI__builtin_neon_vaddv_u16:
  case NEON::BI__builtin_neon_vaddv_u32:
  case NEON::BI__builtin_neon_vaddvq_u8:
  case NEON::BI__builtin_neon_vaddvq_u16:
  case NEON::BI__builtin_neon_vaddvq_u32:
  case NEON::BI__builtin_neon_vaddvq_u64:
    Int = Intrinsic::aarch64_neon_vaddv;
    s = "vaddv"; IntTypes = VectorRet | VectorCastArg1; break;
  case NEON::BI__builtin_neon_vmaxvq_f32:
    Int = Intrinsic::aarch64_neon_vmaxv;
    s = "vmaxv"; break;
  case NEON::BI__builtin_neon_vminvq_f32:
    Int = Intrinsic::aarch64_neon_vminv;
    s = "vminv"; break;
  case NEON::BI__builtin_neon_vmaxnmvq_f32:
    Int = Intrinsic::aarch64_neon_vmaxnmv;
    s = "vmaxnmv"; break;
  case NEON::BI__builtin_neon_vminnmvq_f32:
    Int = Intrinsic::aarch64_neon_vminnmv;
     s = "vminnmv"; break;
  // Scalar Integer Saturating Doubling Multiply Half High
  case NEON::BI__builtin_neon_vqdmulhh_s16:
  case NEON::BI__builtin_neon_vqdmulhs_s32:
    Int = Intrinsic::arm_neon_vqdmulh;
    s = "vqdmulh"; IntTypes = VectorRet; break;
  // Scalar Integer Saturating Rounding Doubling Multiply Half High
  case NEON::BI__builtin_neon_vqrdmulhh_s16:
  case NEON::BI__builtin_neon_vqrdmulhs_s32:
    Int = Intrinsic::arm_neon_vqrdmulh;
    s = "vqrdmulh"; IntTypes = VectorRet; break;
  // Scalar Floating-point Reciprocal Step
  case NEON::BI__builtin_neon_vrecpss_f32:
  case NEON::BI__builtin_neon_vrecpsd_f64:
    Int = Intrinsic::aarch64_neon_vrecps;
    s = "vrecps"; IntTypes = ScalarRet; break;
  // Scalar Floating-point Reciprocal Square Root Step
  case NEON::BI__builtin_neon_vrsqrtss_f32:
  case NEON::BI__builtin_neon_vrsqrtsd_f64:
    Int = Intrinsic::aarch64_neon_vrsqrts;
    s = "vrsqrts"; IntTypes = ScalarRet; break;
  // Scalar Signed Integer Convert To Floating-point
  case NEON::BI__builtin_neon_vcvts_f32_s32:
  case NEON::BI__builtin_neon_vcvtd_f64_s64:
    Int = Intrinsic::aarch64_neon_vcvtint2fps;
    s = "vcvtf"; IntTypes = ScalarRet | VectorGetArg0; break;
  // Scalar Unsigned Integer Convert To Floating-point
  case NEON::BI__builtin_neon_vcvts_f32_u32:
  case NEON::BI__builtin_neon_vcvtd_f64_u64:
    Int = Intrinsic::aarch64_neon_vcvtint2fpu;
    s = "vcvtf"; IntTypes = ScalarRet | VectorGetArg0; break;
  // Scalar Floating-point Converts
  case NEON::BI__builtin_neon_vcvtxd_f32_f64:
    Int = Intrinsic::aarch64_neon_fcvtxn;
    s = "vcvtxn"; break;
  case NEON::BI__builtin_neon_vcvtas_s32_f32:
  case NEON::BI__builtin_neon_vcvtad_s64_f64:
    Int = Intrinsic::aarch64_neon_fcvtas;
    s = "vcvtas"; IntTypes = VectorRet | ScalarArg1; break;
  case NEON::BI__builtin_neon_vcvtas_u32_f32:
  case NEON::BI__builtin_neon_vcvtad_u64_f64:
    Int = Intrinsic::aarch64_neon_fcvtau;
    s = "vcvtau"; IntTypes = VectorRet | ScalarArg1; break;
  case NEON::BI__builtin_neon_vcvtms_s32_f32:
  case NEON::BI__builtin_neon_vcvtmd_s64_f64:
    Int = Intrinsic::aarch64_neon_fcvtms;
    s = "vcvtms"; IntTypes = VectorRet | ScalarArg1; break;
  case NEON::BI__builtin_neon_vcvtms_u32_f32:
  case NEON::BI__builtin_neon_vcvtmd_u64_f64:
    Int = Intrinsic::aarch64_neon_fcvtmu;
    s = "vcvtmu"; IntTypes = VectorRet | ScalarArg1; break;
  case NEON::BI__builtin_neon_vcvtns_s32_f32:
  case NEON::BI__builtin_neon_vcvtnd_s64_f64:
    Int = Intrinsic::aarch64_neon_fcvtns;
    s = "vcvtns"; IntTypes = VectorRet | ScalarArg1; break;
  case NEON::BI__builtin_neon_vcvtns_u32_f32:
  case NEON::BI__builtin_neon_vcvtnd_u64_f64:
    Int = Intrinsic::aarch64_neon_fcvtnu;
    s = "vcvtnu"; IntTypes = VectorRet | ScalarArg1; break;
  case NEON::BI__builtin_neon_vcvtps_s32_f32:
  case NEON::BI__builtin_neon_vcvtpd_s64_f64:
    Int = Intrinsic::aarch64_neon_fcvtps;
    s = "vcvtps"; IntTypes = VectorRet | ScalarArg1; break;
  case NEON::BI__builtin_neon_vcvtps_u32_f32:
  case NEON::BI__builtin_neon_vcvtpd_u64_f64:
    Int = Intrinsic::aarch64_neon_fcvtpu;
    s = "vcvtpu"; IntTypes = VectorRet | ScalarArg1; break;
  case NEON::BI__builtin_neon_vcvts_s32_f32:
  case NEON::BI__builtin_neon_vcvtd_s64_f64:
    Int = Intrinsic::aarch64_neon_fcvtzs;
    s = "vcvtzs"; IntTypes = VectorRet | ScalarArg1; break;
  case NEON::BI__builtin_neon_vcvts_u32_f32:
  case NEON::BI__builtin_neon_vcvtd_u64_f64:
    Int = Intrinsic::aarch64_neon_fcvtzu;
    s = "vcvtzu"; IntTypes = VectorRet | ScalarArg1; break;
  // Scalar Floating-point Reciprocal Estimate
  case NEON::BI__builtin_neon_vrecpes_f32:
  case NEON::BI__builtin_neon_vrecped_f64:
    Int = Intrinsic::aarch64_neon_vrecpe;
    s = "vrecpe"; IntTypes = ScalarRet; break;
  // Scalar Floating-point Reciprocal Exponent
  case NEON::BI__builtin_neon_vrecpxs_f32:
  case NEON::BI__builtin_neon_vrecpxd_f64:
    Int = Intrinsic::aarch64_neon_vrecpx;
    s = "vrecpx"; IntTypes = ScalarRet; break;
  // Scalar Floating-point Reciprocal Square Root Estimate
  case NEON::BI__builtin_neon_vrsqrtes_f32:
  case NEON::BI__builtin_neon_vrsqrted_f64:
    Int = Intrinsic::aarch64_neon_vrsqrte;
    s = "vrsqrte"; IntTypes = ScalarRet; break;
  // Scalar Compare Equal
  case NEON::BI__builtin_neon_vceqd_s64:
  case NEON::BI__builtin_neon_vceqd_u64:
    Int = Intrinsic::aarch64_neon_vceq; s = "vceq";
    IntTypes = VectorRet | VectorGetArg0 | VectorGetArg1; break;
  // Scalar Compare Equal To Zero
  case NEON::BI__builtin_neon_vceqzd_s64:
  case NEON::BI__builtin_neon_vceqzd_u64:
    Int = Intrinsic::aarch64_neon_vceq; s = "vceq";
    // Add implicit zero operand.
    Ops.push_back(llvm::Constant::getNullValue(Ops[0]->getType()));
    IntTypes = VectorRet | VectorGetArg0 | VectorGetArg1; break;
  // Scalar Compare Greater Than or Equal
  case NEON::BI__builtin_neon_vcged_s64:
    Int = Intrinsic::aarch64_neon_vcge; s = "vcge";
    IntTypes = VectorRet | VectorGetArg0 | VectorGetArg1; break;
  case NEON::BI__builtin_neon_vcged_u64:
    Int = Intrinsic::aarch64_neon_vchs; s = "vcge";
    IntTypes = VectorRet | VectorGetArg0 | VectorGetArg1; break;
  // Scalar Compare Greater Than or Equal To Zero
  case NEON::BI__builtin_neon_vcgezd_s64:
    Int = Intrinsic::aarch64_neon_vcge; s = "vcge";
    // Add implicit zero operand.
    Ops.push_back(llvm::Constant::getNullValue(Ops[0]->getType()));
    IntTypes = VectorRet | VectorGetArg0 | VectorGetArg1; break;
  // Scalar Compare Greater Than
  case NEON::BI__builtin_neon_vcgtd_s64:
    Int = Intrinsic::aarch64_neon_vcgt; s = "vcgt";
    IntTypes = VectorRet | VectorGetArg0 | VectorGetArg1; break;
  case NEON::BI__builtin_neon_vcgtd_u64:
    Int = Intrinsic::aarch64_neon_vchi; s = "vcgt";
    IntTypes = VectorRet | VectorGetArg0 | VectorGetArg1; break;
  // Scalar Compare Greater Than Zero
  case NEON::BI__builtin_neon_vcgtzd_s64:
    Int = Intrinsic::aarch64_neon_vcgt; s = "vcgt";
    // Add implicit zero operand.
    Ops.push_back(llvm::Constant::getNullValue(Ops[0]->getType()));
    IntTypes = VectorRet | VectorGetArg0 | VectorGetArg1; break;
  // Scalar Compare Less Than or Equal
  case NEON::BI__builtin_neon_vcled_s64:
    Int = Intrinsic::aarch64_neon_vcge; s = "vcge";
    std::swap(Ops[0], Ops[1]);
    IntTypes = VectorRet | VectorGetArg0 | VectorGetArg1; break;
  case NEON::BI__builtin_neon_vcled_u64:
    Int = Intrinsic::aarch64_neon_vchs; s = "vchs";
    std::swap(Ops[0], Ops[1]);
    IntTypes = VectorRet | VectorGetArg0 | VectorGetArg1; break;
  // Scalar Compare Less Than or Equal To Zero
  case NEON::BI__builtin_neon_vclezd_s64:
    Int = Intrinsic::aarch64_neon_vclez; s = "vcle";
    // Add implicit zero operand.
    Ops.push_back(llvm::Constant::getNullValue(Ops[0]->getType()));
    IntTypes = VectorRet | VectorGetArg0 | VectorGetArg1; break;
  // Scalar Compare Less Than
  case NEON::BI__builtin_neon_vcltd_s64:
    Int = Intrinsic::aarch64_neon_vcgt; s = "vcgt";
    std::swap(Ops[0], Ops[1]);
    IntTypes = VectorRet | VectorGetArg0 | VectorGetArg1; break;
  case NEON::BI__builtin_neon_vcltd_u64:
    Int = Intrinsic::aarch64_neon_vchi; s = "vchi";
    std::swap(Ops[0], Ops[1]);
    IntTypes = VectorRet | VectorGetArg0 | VectorGetArg1; break;
  // Scalar Compare Less Than Zero
  case NEON::BI__builtin_neon_vcltzd_s64:
    Int = Intrinsic::aarch64_neon_vcltz; s = "vclt";
    // Add implicit zero operand.
    Ops.push_back(llvm::Constant::getNullValue(Ops[0]->getType()));
    IntTypes = VectorRet | VectorGetArg0 | VectorGetArg1; break;
  // Scalar Floating-point Compare Equal
  case NEON::BI__builtin_neon_vceqs_f32:
  case NEON::BI__builtin_neon_vceqd_f64:
    Int = Intrinsic::aarch64_neon_fceq; s = "vceq";
    IntTypes = VectorRet | ScalarArg0 | ScalarArg1; break;
  // Scalar Floating-point Compare Equal To Zero
  case NEON::BI__builtin_neon_vceqzs_f32:
  case NEON::BI__builtin_neon_vceqzd_f64:
    Int = Intrinsic::aarch64_neon_fceq; s = "vceq";
    // Add implicit zero operand.
    Ops.push_back(llvm::Constant::getNullValue(CGF.FloatTy));
    IntTypes = VectorRet | ScalarArg0 | ScalarFpCmpzArg1; break;
  // Scalar Floating-point Compare Greater Than Or Equal
  case NEON::BI__builtin_neon_vcges_f32:
  case NEON::BI__builtin_neon_vcged_f64:
    Int = Intrinsic::aarch64_neon_fcge; s = "vcge";
    IntTypes = VectorRet | ScalarArg0 | ScalarArg1; break;
  // Scalar Floating-point Compare Greater Than Or Equal To Zero
  case NEON::BI__builtin_neon_vcgezs_f32:
  case NEON::BI__builtin_neon_vcgezd_f64:
    Int = Intrinsic::aarch64_neon_fcge; s = "vcge";
    // Add implicit zero operand.
    Ops.push_back(llvm::Constant::getNullValue(CGF.FloatTy));
    IntTypes = VectorRet | ScalarArg0 | ScalarFpCmpzArg1; break;
  // Scalar Floating-point Compare Greather Than
  case NEON::BI__builtin_neon_vcgts_f32:
  case NEON::BI__builtin_neon_vcgtd_f64:
    Int = Intrinsic::aarch64_neon_fcgt; s = "vcgt";
    IntTypes = VectorRet | ScalarArg0 | ScalarArg1; break;
  // Scalar Floating-point Compare Greather Than Zero
  case NEON::BI__builtin_neon_vcgtzs_f32:
  case NEON::BI__builtin_neon_vcgtzd_f64:
    Int = Intrinsic::aarch64_neon_fcgt; s = "vcgt";
    // Add implicit zero operand.
    Ops.push_back(llvm::Constant::getNullValue(CGF.FloatTy));
    IntTypes = VectorRet | ScalarArg0 | ScalarFpCmpzArg1; break;
  // Scalar Floating-point Compare Less Than or Equal
  case NEON::BI__builtin_neon_vcles_f32:
  case NEON::BI__builtin_neon_vcled_f64:
    Int = Intrinsic::aarch64_neon_fcge; s = "vcge";
    std::swap(Ops[0], Ops[1]);
    IntTypes = VectorRet | ScalarArg0 | ScalarArg1; break;
  // Scalar Floating-point Compare Less Than Or Equal To Zero
  case NEON::BI__builtin_neon_vclezs_f32:
  case NEON::BI__builtin_neon_vclezd_f64:
    Int = Intrinsic::aarch64_neon_fclez; s = "vcle";
    // Add implicit zero operand.
    Ops.push_back(llvm::Constant::getNullValue(CGF.FloatTy));
    IntTypes = VectorRet | ScalarArg0 | ScalarFpCmpzArg1; break;
  // Scalar Floating-point Compare Less Than Zero
  case NEON::BI__builtin_neon_vclts_f32:
  case NEON::BI__builtin_neon_vcltd_f64:
    Int = Intrinsic::aarch64_neon_fcgt; s = "vcgt";
    std::swap(Ops[0], Ops[1]);
    IntTypes = VectorRet | ScalarArg0 | ScalarArg1; break;
  // Scalar Floating-point Compare Less Than Zero
  case NEON::BI__builtin_neon_vcltzs_f32:
  case NEON::BI__builtin_neon_vcltzd_f64:
    Int = Intrinsic::aarch64_neon_fcltz; s = "vclt";
    // Add implicit zero operand.
    Ops.push_back(llvm::Constant::getNullValue(CGF.FloatTy));
    IntTypes = VectorRet | ScalarArg0 | ScalarFpCmpzArg1; break;
  // Scalar Floating-point Absolute Compare Greater Than Or Equal
  case NEON::BI__builtin_neon_vcages_f32:
  case NEON::BI__builtin_neon_vcaged_f64:
    Int = Intrinsic::aarch64_neon_fcage; s = "vcage";
    IntTypes = VectorRet | ScalarArg0 | ScalarArg1; break;
  // Scalar Floating-point Absolute Compare Greater Than
  case NEON::BI__builtin_neon_vcagts_f32:
  case NEON::BI__builtin_neon_vcagtd_f64:
    Int = Intrinsic::aarch64_neon_fcagt; s = "vcagt";
    IntTypes = VectorRet | ScalarArg0 | ScalarArg1; break;
  // Scalar Floating-point Absolute Compare Less Than Or Equal
  case NEON::BI__builtin_neon_vcales_f32:
  case NEON::BI__builtin_neon_vcaled_f64:
    Int = Intrinsic::aarch64_neon_fcage; s = "vcage";
    std::swap(Ops[0], Ops[1]);
    IntTypes = VectorRet | ScalarArg0 | ScalarArg1; break;
  // Scalar Floating-point Absolute Compare Less Than
  case NEON::BI__builtin_neon_vcalts_f32:
  case NEON::BI__builtin_neon_vcaltd_f64:
    Int = Intrinsic::aarch64_neon_fcagt; s = "vcalt";
    std::swap(Ops[0], Ops[1]);
    IntTypes = VectorRet | ScalarArg0 | ScalarArg1; break;
  // Scalar Compare Bitwise Test Bits
  case NEON::BI__builtin_neon_vtstd_s64:
  case NEON::BI__builtin_neon_vtstd_u64:
    Int = Intrinsic::aarch64_neon_vtstd; s = "vtst";
    IntTypes = VectorRet | VectorGetArg0 | VectorGetArg1; break;
  // Scalar Absolute Value
  case NEON::BI__builtin_neon_vabsd_s64:
    Int = Intrinsic::aarch64_neon_vabs;
    s = "vabs"; break;
  // Scalar Absolute Difference
  case NEON::BI__builtin_neon_vabds_f32:
  case NEON::BI__builtin_neon_vabdd_f64:
    Int = Intrinsic::aarch64_neon_vabd;
    s = "vabd"; IntTypes = ScalarRet; break;
  // Scalar Signed Saturating Absolute Value
  case NEON::BI__builtin_neon_vqabsb_s8:
  case NEON::BI__builtin_neon_vqabsh_s16:
  case NEON::BI__builtin_neon_vqabss_s32:
  case NEON::BI__builtin_neon_vqabsd_s64:
    Int = Intrinsic::arm_neon_vqabs;
    s = "vqabs"; IntTypes = VectorRet; break;
  // Scalar Negate
  case NEON::BI__builtin_neon_vnegd_s64:
    Int = Intrinsic::aarch64_neon_vneg;
    s = "vneg"; break;
  // Scalar Signed Saturating Negate
  case NEON::BI__builtin_neon_vqnegb_s8:
  case NEON::BI__builtin_neon_vqnegh_s16:
  case NEON::BI__builtin_neon_vqnegs_s32:
  case NEON::BI__builtin_neon_vqnegd_s64:
    Int = Intrinsic::arm_neon_vqneg;
    s = "vqneg"; IntTypes = VectorRet; break;
  // Scalar Signed Saturating Accumulated of Unsigned Value
  case NEON::BI__builtin_neon_vuqaddb_s8:
  case NEON::BI__builtin_neon_vuqaddh_s16:
  case NEON::BI__builtin_neon_vuqadds_s32:
  case NEON::BI__builtin_neon_vuqaddd_s64:
    Int = Intrinsic::aarch64_neon_vuqadd;
    s = "vuqadd"; IntTypes = VectorRet; break;
  // Scalar Unsigned Saturating Accumulated of Signed Value
  case NEON::BI__builtin_neon_vsqaddb_u8:
  case NEON::BI__builtin_neon_vsqaddh_u16:
  case NEON::BI__builtin_neon_vsqadds_u32:
  case NEON::BI__builtin_neon_vsqaddd_u64:
    Int = Intrinsic::aarch64_neon_vsqadd;
    s = "vsqadd"; IntTypes = VectorRet; break;
  // Signed Saturating Doubling Multiply-Add Long
  case NEON::BI__builtin_neon_vqdmlalh_s16:
  case NEON::BI__builtin_neon_vqdmlals_s32:
    Int = Intrinsic::aarch64_neon_vqdmlal;
    s = "vqdmlal"; IntTypes = VectorRet; break;
  // Signed Saturating Doubling Multiply-Subtract Long
  case NEON::BI__builtin_neon_vqdmlslh_s16:
  case NEON::BI__builtin_neon_vqdmlsls_s32:
    Int = Intrinsic::aarch64_neon_vqdmlsl;
    s = "vqdmlsl"; IntTypes = VectorRet; break;
  // Signed Saturating Doubling Multiply Long
  case NEON::BI__builtin_neon_vqdmullh_s16:
  case NEON::BI__builtin_neon_vqdmulls_s32:
    Int = Intrinsic::arm_neon_vqdmull;
    s = "vqdmull"; IntTypes = VectorRet; break;
  // Scalar Signed Saturating Extract Unsigned Narrow
  case NEON::BI__builtin_neon_vqmovunh_s16:
  case NEON::BI__builtin_neon_vqmovuns_s32:
  case NEON::BI__builtin_neon_vqmovund_s64:
    Int = Intrinsic::arm_neon_vqmovnsu;
    s = "vqmovun"; IntTypes = VectorRet; break;
  // Scalar Signed Saturating Extract Narrow
  case NEON::BI__builtin_neon_vqmovnh_s16:
  case NEON::BI__builtin_neon_vqmovns_s32:
  case NEON::BI__builtin_neon_vqmovnd_s64:
    Int = Intrinsic::arm_neon_vqmovns;
    s = "vqmovn"; IntTypes = VectorRet; break;
  // Scalar Unsigned Saturating Extract Narrow
  case NEON::BI__builtin_neon_vqmovnh_u16:
  case NEON::BI__builtin_neon_vqmovns_u32:
  case NEON::BI__builtin_neon_vqmovnd_u64:
    Int = Intrinsic::arm_neon_vqmovnu;
    s = "vqmovn"; IntTypes = VectorRet; break;
  // Scalar Signed Shift Right (Immediate)
  case NEON::BI__builtin_neon_vshrd_n_s64:
    Int = Intrinsic::aarch64_neon_vshrds_n;
    s = "vsshr"; break;
  // Scalar Unsigned Shift Right (Immediate)
  case NEON::BI__builtin_neon_vshrd_n_u64:
    Int = Intrinsic::aarch64_neon_vshrdu_n;
    s = "vushr"; break;
  // Scalar Signed Rounding Shift Right (Immediate)
  case NEON::BI__builtin_neon_vrshrd_n_s64:
    Int = Intrinsic::aarch64_neon_vsrshr;
    s = "vsrshr"; IntTypes = VectorRet; break;
  // Scalar Unsigned Rounding Shift Right (Immediate)
  case NEON::BI__builtin_neon_vrshrd_n_u64:
    Int = Intrinsic::aarch64_neon_vurshr;
    s = "vurshr"; IntTypes = VectorRet; break;
  // Scalar Signed Shift Right and Accumulate (Immediate)
  case NEON::BI__builtin_neon_vsrad_n_s64:
    Int = Intrinsic::aarch64_neon_vsrads_n;
    s = "vssra"; break;
  // Scalar Unsigned Shift Right and Accumulate (Immediate)
  case NEON::BI__builtin_neon_vsrad_n_u64:
    Int = Intrinsic::aarch64_neon_vsradu_n;
    s = "vusra"; break;
  // Scalar Signed Rounding Shift Right and Accumulate (Immediate)
  case NEON::BI__builtin_neon_vrsrad_n_s64:
    Int = Intrinsic::aarch64_neon_vrsrads_n;
    s = "vsrsra"; break;
  // Scalar Unsigned Rounding Shift Right and Accumulate (Immediate)
  case NEON::BI__builtin_neon_vrsrad_n_u64:
    Int = Intrinsic::aarch64_neon_vrsradu_n;
    s = "vursra"; break;
  // Scalar Signed/Unsigned Shift Left (Immediate)
  case NEON::BI__builtin_neon_vshld_n_s64:
  case NEON::BI__builtin_neon_vshld_n_u64:
    Int = Intrinsic::aarch64_neon_vshld_n;
    s = "vshl"; break;
  // Signed Saturating Shift Left (Immediate)
  case NEON::BI__builtin_neon_vqshlb_n_s8:
  case NEON::BI__builtin_neon_vqshlh_n_s16:
  case NEON::BI__builtin_neon_vqshls_n_s32:
  case NEON::BI__builtin_neon_vqshld_n_s64:
    Int = Intrinsic::aarch64_neon_vqshls_n;
    s = "vsqshl"; IntTypes = VectorRet; break;
  // Unsigned Saturating Shift Left (Immediate)
  case NEON::BI__builtin_neon_vqshlb_n_u8:
  case NEON::BI__builtin_neon_vqshlh_n_u16:
  case NEON::BI__builtin_neon_vqshls_n_u32:
  case NEON::BI__builtin_neon_vqshld_n_u64:
    Int = Intrinsic::aarch64_neon_vqshlu_n;
    s = "vuqshl"; IntTypes = VectorRet; break;
  // Signed Saturating Shift Left Unsigned (Immediate)
  case NEON::BI__builtin_neon_vqshlub_n_s8:
  case NEON::BI__builtin_neon_vqshluh_n_s16:
  case NEON::BI__builtin_neon_vqshlus_n_s32:
  case NEON::BI__builtin_neon_vqshlud_n_s64:
    Int = Intrinsic::aarch64_neon_vsqshlu;
    s = "vsqshlu"; IntTypes = VectorRet; break;
  // Shift Right And Insert (Immediate)
  case NEON::BI__builtin_neon_vsrid_n_s64:
  case NEON::BI__builtin_neon_vsrid_n_u64:
    Int = Intrinsic::aarch64_neon_vsri;
    s = "vsri"; IntTypes = VectorRet; break;
  // Shift Left And Insert (Immediate)
  case NEON::BI__builtin_neon_vslid_n_s64:
  case NEON::BI__builtin_neon_vslid_n_u64:
    Int = Intrinsic::aarch64_neon_vsli;
    s = "vsli"; IntTypes = VectorRet; break;
  // Signed Saturating Shift Right Narrow (Immediate)
  case NEON::BI__builtin_neon_vqshrnh_n_s16:
  case NEON::BI__builtin_neon_vqshrns_n_s32:
  case NEON::BI__builtin_neon_vqshrnd_n_s64:
    Int = Intrinsic::aarch64_neon_vsqshrn;
    s = "vsqshrn"; IntTypes = VectorRet; break;
  // Unsigned Saturating Shift Right Narrow (Immediate)
  case NEON::BI__builtin_neon_vqshrnh_n_u16:
  case NEON::BI__builtin_neon_vqshrns_n_u32:
  case NEON::BI__builtin_neon_vqshrnd_n_u64:
    Int = Intrinsic::aarch64_neon_vuqshrn;
    s = "vuqshrn"; IntTypes = VectorRet; break;
  // Signed Saturating Rounded Shift Right Narrow (Immediate)
  case NEON::BI__builtin_neon_vqrshrnh_n_s16:
  case NEON::BI__builtin_neon_vqrshrns_n_s32:
  case NEON::BI__builtin_neon_vqrshrnd_n_s64:
    Int = Intrinsic::aarch64_neon_vsqrshrn;
    s = "vsqrshrn"; IntTypes = VectorRet; break;
  // Unsigned Saturating Rounded Shift Right Narrow (Immediate)
  case NEON::BI__builtin_neon_vqrshrnh_n_u16:
  case NEON::BI__builtin_neon_vqrshrns_n_u32:
  case NEON::BI__builtin_neon_vqrshrnd_n_u64:
    Int = Intrinsic::aarch64_neon_vuqrshrn;
    s = "vuqrshrn"; IntTypes = VectorRet; break;
  // Signed Saturating Shift Right Unsigned Narrow (Immediate)
  case NEON::BI__builtin_neon_vqshrunh_n_s16:
  case NEON::BI__builtin_neon_vqshruns_n_s32:
  case NEON::BI__builtin_neon_vqshrund_n_s64:
    Int = Intrinsic::aarch64_neon_vsqshrun;
    s = "vsqshrun"; IntTypes = VectorRet; break;
  // Signed Saturating Rounded Shift Right Unsigned Narrow (Immediate)
  case NEON::BI__builtin_neon_vqrshrunh_n_s16:
  case NEON::BI__builtin_neon_vqrshruns_n_s32:
  case NEON::BI__builtin_neon_vqrshrund_n_s64:
    Int = Intrinsic::aarch64_neon_vsqrshrun;
    s = "vsqrshrun"; IntTypes = VectorRet; break;
  // Scalar Signed Fixed-point Convert To Floating-Point (Immediate)
  case NEON::BI__builtin_neon_vcvts_n_f32_s32:
  case NEON::BI__builtin_neon_vcvtd_n_f64_s64:
    Int = Intrinsic::aarch64_neon_vcvtfxs2fp_n;
    s = "vcvtf"; IntTypes = ScalarRet | VectorGetArg0; break;
  // Scalar Unsigned Fixed-point Convert To Floating-Point (Immediate)
  case NEON::BI__builtin_neon_vcvts_n_f32_u32:
  case NEON::BI__builtin_neon_vcvtd_n_f64_u64:
    Int = Intrinsic::aarch64_neon_vcvtfxu2fp_n;
    s = "vcvtf"; IntTypes = ScalarRet | VectorGetArg0; break;
  // Scalar Floating-point Convert To Signed Fixed-point (Immediate)
  case NEON::BI__builtin_neon_vcvts_n_s32_f32:
  case NEON::BI__builtin_neon_vcvtd_n_s64_f64:
    Int = Intrinsic::aarch64_neon_vcvtfp2fxs_n;
    s = "fcvtzs"; IntTypes = VectorRet | ScalarArg0; break;
  // Scalar Floating-point Convert To Unsigned Fixed-point (Immediate)
  case NEON::BI__builtin_neon_vcvts_n_u32_f32:
  case NEON::BI__builtin_neon_vcvtd_n_u64_f64:
    Int = Intrinsic::aarch64_neon_vcvtfp2fxu_n;
    s = "fcvtzu"; IntTypes = VectorRet | ScalarArg0; break;
  case NEON::BI__builtin_neon_vmull_p64:
    Int = Intrinsic::aarch64_neon_vmull_p64;
    s = "vmull"; break;
  }

  if (!Int)
    return 0;

  // Determine the type(s) of this overloaded AArch64 intrinsic.
  Function *F = 0;
  SmallVector<llvm::Type *, 3> Tys;

  // Return type.
  if (IntTypes & (ScalarRet | VectorRet)) {
     llvm::Type *Ty = CGF.ConvertType(E->getCallReturnType());
    if (IntTypes & ScalarRet) {
      // Scalar return value.
      Tys.push_back(Ty);
    } else if (IntTypes & VectorRet) {
      // Convert the scalar return type to one-vector element type.
      Tys.push_back(llvm::VectorType::get(Ty, 1));
    }
  }

  // Arguments.
  if (IntTypes & (ScalarArg0 | VectorGetArg0 | VectorCastArg0)) {
    const Expr *Arg = E->getArg(0);
    llvm::Type *Ty = CGF.ConvertType(Arg->getType());
    if (IntTypes & ScalarArg0) {
      // Scalar argument.
      Tys.push_back(Ty);
    } else if (IntTypes & VectorGetArg0) {
      // Convert the scalar argument to one-vector element type.
      Tys.push_back(llvm::VectorType::get(Ty, 1));
    } else if (IntTypes & VectorCastArg0) {
      // Cast the argument to vector type.
      Tys.push_back(cast<llvm::VectorType>(Ty));
    }
  }
 
  // The only intrinsics that require a 2nd argument are the compare intrinsics.
  // However, the builtins don't always have a 2nd argument (e.g.,
  // floating-point compare to zero), so we inspect the first argument to
  // determine the type.
  if (IntTypes & (ScalarArg1 | VectorGetArg1 | VectorCastArg1)) {
    const Expr *Arg = E->getArg(0);
    llvm::Type *Ty = CGF.ConvertType(Arg->getType());
    if (IntTypes & ScalarArg1) {
      // Scalar argument.
      Tys.push_back(Ty);
    } else if (IntTypes & VectorGetArg1) {
      // Convert the scalar argument to one-vector element type.
      Tys.push_back(llvm::VectorType::get(Ty, 1));
    } else if (IntTypes & VectorCastArg1) {
      // Cast the argument to a vector type.
      Tys.push_back(cast<llvm::VectorType>(Ty));
    }
  } else if (IntTypes & ScalarFpCmpzArg1) {
    // Floating-point zero argument.
    Tys.push_back(CGF.FloatTy);
  }
 
  if (IntTypes)
     F = CGF.CGM.getIntrinsic(Int, Tys);
  else
     F = CGF.CGM.getIntrinsic(Int);

  Value *Result = CGF.EmitNeonCall(F, Ops, s);
  llvm::Type *ResultType = CGF.ConvertType(E->getType());
  // AArch64 intrinsic one-element vector type cast to
  // scalar type expected by the builtin
  return CGF.Builder.CreateBitCast(Result, ResultType, s);
}

Value *CodeGenFunction::EmitAArch64CompareBuiltinExpr(
    Value *Op, llvm::Type *Ty, const CmpInst::Predicate Fp,
    const CmpInst::Predicate Ip, const Twine &Name) {
  llvm::Type *OTy = ((llvm::User *)Op)->getOperand(0)->getType();
  if (OTy->isPointerTy())
    OTy = Ty;
  Op = Builder.CreateBitCast(Op, OTy);
  if (((llvm::VectorType *)OTy)->getElementType()->isFloatingPointTy()) {
    Op = Builder.CreateFCmp(Fp, Op, ConstantAggregateZero::get(OTy));
  } else {
    Op = Builder.CreateICmp(Ip, Op, ConstantAggregateZero::get(OTy));
  }
  return Builder.CreateSExt(Op, Ty, Name);
}

static Value *packTBLDVectorList(CodeGenFunction &CGF, ArrayRef<Value *> Ops,
                                 Value *ExtOp, Value *IndexOp,
                                 llvm::Type *ResTy, unsigned IntID,
                                 const char *Name) {
  SmallVector<Value *, 2> TblOps;
  if (ExtOp)
    TblOps.push_back(ExtOp);

  // Build a vector containing sequential number like (0, 1, 2, ..., 15)  
  SmallVector<Constant*, 16> Indices;
  llvm::VectorType *TblTy = cast<llvm::VectorType>(Ops[0]->getType());
  for (unsigned i = 0, e = TblTy->getNumElements(); i != e; ++i) {
    Indices.push_back(ConstantInt::get(CGF.Int32Ty, 2*i));
    Indices.push_back(ConstantInt::get(CGF.Int32Ty, 2*i+1));
  }
  Value *SV = llvm::ConstantVector::get(Indices);

  int PairPos = 0, End = Ops.size() - 1;
  while (PairPos < End) {
    TblOps.push_back(CGF.Builder.CreateShuffleVector(Ops[PairPos],
                                                     Ops[PairPos+1], SV, Name));
    PairPos += 2;
  }

  // If there's an odd number of 64-bit lookup table, fill the high 64-bit
  // of the 128-bit lookup table with zero.
  if (PairPos == End) {
    Value *ZeroTbl = ConstantAggregateZero::get(TblTy);
    TblOps.push_back(CGF.Builder.CreateShuffleVector(Ops[PairPos],
                                                     ZeroTbl, SV, Name));
  }

  TblTy = llvm::VectorType::get(TblTy->getElementType(),
                                2*TblTy->getNumElements());
  llvm::Type *Tys[2] = { ResTy, TblTy };

  Function *TblF;
  TblOps.push_back(IndexOp);
  TblF = CGF.CGM.getIntrinsic(IntID, Tys);
  
  return CGF.EmitNeonCall(TblF, TblOps, Name);
}

static Value *EmitAArch64TblBuiltinExpr(CodeGenFunction &CGF,
                                        unsigned BuiltinID,
                                        const CallExpr *E) {
  unsigned int Int = 0;
  const char *s = NULL;

  unsigned TblPos;
  switch (BuiltinID) {
  default:
    return 0;
  case NEON::BI__builtin_neon_vtbl1_v:
  case NEON::BI__builtin_neon_vqtbl1_v:
  case NEON::BI__builtin_neon_vqtbl1q_v:
  case NEON::BI__builtin_neon_vtbl2_v:
  case NEON::BI__builtin_neon_vqtbl2_v:
  case NEON::BI__builtin_neon_vqtbl2q_v:
  case NEON::BI__builtin_neon_vtbl3_v:
  case NEON::BI__builtin_neon_vqtbl3_v:
  case NEON::BI__builtin_neon_vqtbl3q_v:
  case NEON::BI__builtin_neon_vtbl4_v:
  case NEON::BI__builtin_neon_vqtbl4_v:
  case NEON::BI__builtin_neon_vqtbl4q_v:
    TblPos = 0;
    break;
  case NEON::BI__builtin_neon_vtbx1_v:
  case NEON::BI__builtin_neon_vqtbx1_v:
  case NEON::BI__builtin_neon_vqtbx1q_v:
  case NEON::BI__builtin_neon_vtbx2_v:
  case NEON::BI__builtin_neon_vqtbx2_v:
  case NEON::BI__builtin_neon_vqtbx2q_v:
  case NEON::BI__builtin_neon_vtbx3_v:
  case NEON::BI__builtin_neon_vqtbx3_v:
  case NEON::BI__builtin_neon_vqtbx3q_v:
  case NEON::BI__builtin_neon_vtbx4_v:
  case NEON::BI__builtin_neon_vqtbx4_v:
  case NEON::BI__builtin_neon_vqtbx4q_v:
    TblPos = 1;
    break;
  }

  assert(E->getNumArgs() >= 3);

  // Get the last argument, which specifies the vector type.
  llvm::APSInt Result;
  const Expr *Arg = E->getArg(E->getNumArgs() - 1);
  if (!Arg->isIntegerConstantExpr(Result, CGF.getContext()))
    return 0;

  // Determine the type of this overloaded NEON intrinsic.
  NeonTypeFlags Type(Result.getZExtValue());
  llvm::VectorType *VTy = GetNeonType(&CGF, Type);
  llvm::Type *Ty = VTy;
  if (!Ty)
    return 0;

  SmallVector<Value *, 4> Ops;
  for (unsigned i = 0, e = E->getNumArgs() - 1; i != e; i++) {
    Ops.push_back(CGF.EmitScalarExpr(E->getArg(i)));
  }

  Arg = E->getArg(TblPos);
  llvm::Type *TblTy = CGF.ConvertType(Arg->getType());
  llvm::VectorType *VTblTy = cast<llvm::VectorType>(TblTy);
  llvm::Type *Tys[2] = { Ty, VTblTy };
  unsigned nElts = VTy->getNumElements();  

  // AArch64 scalar builtins are not overloaded, they do not have an extra
  // argument that specifies the vector type, need to handle each case.
  SmallVector<Value *, 2> TblOps;
  switch (BuiltinID) {
  case NEON::BI__builtin_neon_vtbl1_v: {
    TblOps.push_back(Ops[0]);
    return packTBLDVectorList(CGF, TblOps, 0, Ops[1], Ty,
                              Intrinsic::aarch64_neon_vtbl1, "vtbl1");
  }
  case NEON::BI__builtin_neon_vtbl2_v: {
    TblOps.push_back(Ops[0]);
    TblOps.push_back(Ops[1]);
    return packTBLDVectorList(CGF, TblOps, 0, Ops[2], Ty,
                              Intrinsic::aarch64_neon_vtbl1, "vtbl1");
  }
  case NEON::BI__builtin_neon_vtbl3_v: {
    TblOps.push_back(Ops[0]);
    TblOps.push_back(Ops[1]);
    TblOps.push_back(Ops[2]);
    return packTBLDVectorList(CGF, TblOps, 0, Ops[3], Ty,
                              Intrinsic::aarch64_neon_vtbl2, "vtbl2");
  }
  case NEON::BI__builtin_neon_vtbl4_v: {
    TblOps.push_back(Ops[0]);
    TblOps.push_back(Ops[1]);
    TblOps.push_back(Ops[2]);
    TblOps.push_back(Ops[3]);
    return packTBLDVectorList(CGF, TblOps, 0, Ops[4], Ty,
                              Intrinsic::aarch64_neon_vtbl2, "vtbl2");
  }
  case NEON::BI__builtin_neon_vtbx1_v: {
    TblOps.push_back(Ops[1]);
    Value *TblRes = packTBLDVectorList(CGF, TblOps, 0, Ops[2], Ty,
                                    Intrinsic::aarch64_neon_vtbl1, "vtbl1");

    llvm::Constant *Eight = ConstantInt::get(VTy->getElementType(), 8);
    Value* EightV = llvm::ConstantVector::getSplat(nElts, Eight);
    Value *CmpRes = CGF.Builder.CreateICmp(ICmpInst::ICMP_UGE, Ops[2], EightV);
    CmpRes = CGF.Builder.CreateSExt(CmpRes, Ty);

    SmallVector<Value *, 4> BslOps;
    BslOps.push_back(CmpRes);
    BslOps.push_back(Ops[0]);
    BslOps.push_back(TblRes);
    Function *BslF = CGF.CGM.getIntrinsic(Intrinsic::arm_neon_vbsl, Ty);
    return CGF.EmitNeonCall(BslF, BslOps, "vbsl");
  }
  case NEON::BI__builtin_neon_vtbx2_v: {
    TblOps.push_back(Ops[1]);
    TblOps.push_back(Ops[2]);
    return packTBLDVectorList(CGF, TblOps, Ops[0], Ops[3], Ty,
                              Intrinsic::aarch64_neon_vtbx1, "vtbx1");
  }
  case NEON::BI__builtin_neon_vtbx3_v: {
    TblOps.push_back(Ops[1]);
    TblOps.push_back(Ops[2]);
    TblOps.push_back(Ops[3]);
    Value *TblRes = packTBLDVectorList(CGF, TblOps, 0, Ops[4], Ty,
                                       Intrinsic::aarch64_neon_vtbl2, "vtbl2");

    llvm::Constant *TwentyFour = ConstantInt::get(VTy->getElementType(), 24);
    Value* TwentyFourV = llvm::ConstantVector::getSplat(nElts, TwentyFour);
    Value *CmpRes = CGF.Builder.CreateICmp(ICmpInst::ICMP_UGE, Ops[4],
                                           TwentyFourV);
    CmpRes = CGF.Builder.CreateSExt(CmpRes, Ty);
  
    SmallVector<Value *, 4> BslOps;
    BslOps.push_back(CmpRes);
    BslOps.push_back(Ops[0]);
    BslOps.push_back(TblRes);
    Function *BslF = CGF.CGM.getIntrinsic(Intrinsic::arm_neon_vbsl, Ty);
    return CGF.EmitNeonCall(BslF, BslOps, "vbsl");
  }
  case NEON::BI__builtin_neon_vtbx4_v: {
    TblOps.push_back(Ops[1]);
    TblOps.push_back(Ops[2]);
    TblOps.push_back(Ops[3]);
    TblOps.push_back(Ops[4]);
    return packTBLDVectorList(CGF, TblOps, Ops[0], Ops[5], Ty,
                              Intrinsic::aarch64_neon_vtbx2, "vtbx2");
  }
  case NEON::BI__builtin_neon_vqtbl1_v:
  case NEON::BI__builtin_neon_vqtbl1q_v:
    Int = Intrinsic::aarch64_neon_vtbl1; s = "vtbl1"; break;
  case NEON::BI__builtin_neon_vqtbl2_v:
  case NEON::BI__builtin_neon_vqtbl2q_v: {
    Int = Intrinsic::aarch64_neon_vtbl2; s = "vtbl2"; break;
  case NEON::BI__builtin_neon_vqtbl3_v:
  case NEON::BI__builtin_neon_vqtbl3q_v:
    Int = Intrinsic::aarch64_neon_vtbl3; s = "vtbl3"; break;
  case NEON::BI__builtin_neon_vqtbl4_v:
  case NEON::BI__builtin_neon_vqtbl4q_v:
    Int = Intrinsic::aarch64_neon_vtbl4; s = "vtbl4"; break;
  case NEON::BI__builtin_neon_vqtbx1_v:
  case NEON::BI__builtin_neon_vqtbx1q_v:
    Int = Intrinsic::aarch64_neon_vtbx1; s = "vtbx1"; break;
  case NEON::BI__builtin_neon_vqtbx2_v:
  case NEON::BI__builtin_neon_vqtbx2q_v:
    Int = Intrinsic::aarch64_neon_vtbx2; s = "vtbx2"; break;
  case NEON::BI__builtin_neon_vqtbx3_v:
  case NEON::BI__builtin_neon_vqtbx3q_v:
    Int = Intrinsic::aarch64_neon_vtbx3; s = "vtbx3"; break;
  case NEON::BI__builtin_neon_vqtbx4_v:
  case NEON::BI__builtin_neon_vqtbx4q_v:
    Int = Intrinsic::aarch64_neon_vtbx4; s = "vtbx4"; break;
  }
  }

  if (!Int)
    return 0;

  Function *F = CGF.CGM.getIntrinsic(Int, Tys);
  return CGF.EmitNeonCall(F, Ops, s);
}

#define NEONMAP0(ClangBuiltin) { NEON::BI ## ClangBuiltin, 0, 0 }
#define NEONMAP1(ClangBuiltin, LLVMInt) \
  { NEON::BI ## ClangBuiltin, Intrinsic::LLVMInt, 0 }
#define NEONMAP2(ClangBuiltin, LLVMInt, AltLLVMInt) \
  { NEON::BI ## ClangBuiltin, Intrinsic::LLVMInt, Intrinsic::AltLLVMInt }

static CodeGenFunction::NeonIntrinsicMap ARMNeonIntrinsicMap [] = {
  NEONMAP0(__builtin_neon_vaddhn_v),
  NEONMAP1(__builtin_neon_vaesdq_v, arm_neon_aesd),
  NEONMAP1(__builtin_neon_vaeseq_v, arm_neon_aese),
  NEONMAP1(__builtin_neon_vaesimcq_v, arm_neon_aesimc),
  NEONMAP1(__builtin_neon_vaesmcq_v, arm_neon_aesmc),
  NEONMAP1(__builtin_neon_vcage_v, arm_neon_vacge),
  NEONMAP1(__builtin_neon_vcageq_v, arm_neon_vacge),
  NEONMAP1(__builtin_neon_vcagt_v, arm_neon_vacgt),
  NEONMAP1(__builtin_neon_vcagtq_v, arm_neon_vacgt),
  NEONMAP1(__builtin_neon_vcale_v, arm_neon_vacge),
  NEONMAP1(__builtin_neon_vcaleq_v, arm_neon_vacge),
  NEONMAP1(__builtin_neon_vcalt_v, arm_neon_vacgt),
  NEONMAP1(__builtin_neon_vcaltq_v, arm_neon_vacgt),
  NEONMAP0(__builtin_neon_vext_v),
  NEONMAP0(__builtin_neon_vextq_v),
  NEONMAP0(__builtin_neon_vfma_v),
  NEONMAP0(__builtin_neon_vfmaq_v),
  NEONMAP2(__builtin_neon_vhadd_v, arm_neon_vhaddu, arm_neon_vhadds),
  NEONMAP2(__builtin_neon_vhaddq_v, arm_neon_vhaddu, arm_neon_vhadds),
  NEONMAP2(__builtin_neon_vhsub_v, arm_neon_vhsubu, arm_neon_vhsubs),
  NEONMAP2(__builtin_neon_vhsubq_v, arm_neon_vhsubu, arm_neon_vhsubs),
  NEONMAP0(__builtin_neon_vmovl_v),
  NEONMAP0(__builtin_neon_vmovn_v),
  NEONMAP1(__builtin_neon_vmul_v, arm_neon_vmulp),
  NEONMAP1(__builtin_neon_vmulq_v, arm_neon_vmulp),
  NEONMAP1(__builtin_neon_vpadd_v, arm_neon_vpadd),
  NEONMAP2(__builtin_neon_vpaddl_v, arm_neon_vpaddlu, arm_neon_vpaddls),
  NEONMAP2(__builtin_neon_vpaddlq_v, arm_neon_vpaddlu, arm_neon_vpaddls),
  NEONMAP1(__builtin_neon_vpaddq_v, arm_neon_vpadd),
  NEONMAP2(__builtin_neon_vqadd_v, arm_neon_vqaddu, arm_neon_vqadds),
  NEONMAP2(__builtin_neon_vqaddq_v, arm_neon_vqaddu, arm_neon_vqadds),
  NEONMAP1(__builtin_neon_vqdmulh_v, arm_neon_vqdmulh),
  NEONMAP1(__builtin_neon_vqdmulhq_v, arm_neon_vqdmulh),
  NEONMAP1(__builtin_neon_vqdmull_v, arm_neon_vqdmull),
  NEONMAP1(__builtin_neon_vqrdmulh_v, arm_neon_vqrdmulh),
  NEONMAP1(__builtin_neon_vqrdmulhq_v, arm_neon_vqrdmulh),
  NEONMAP2(__builtin_neon_vqrshl_v, arm_neon_vqrshiftu, arm_neon_vqrshifts),
  NEONMAP2(__builtin_neon_vqrshlq_v, arm_neon_vqrshiftu, arm_neon_vqrshifts),
  NEONMAP2(__builtin_neon_vqshl_n_v, arm_neon_vqshiftu, arm_neon_vqshifts),
  NEONMAP2(__builtin_neon_vqshl_v, arm_neon_vqshiftu, arm_neon_vqshifts),
  NEONMAP2(__builtin_neon_vqshlq_n_v, arm_neon_vqshiftu, arm_neon_vqshifts),
  NEONMAP2(__builtin_neon_vqshlq_v, arm_neon_vqshiftu, arm_neon_vqshifts),
  NEONMAP2(__builtin_neon_vqsub_v, arm_neon_vqsubu, arm_neon_vqsubs),
  NEONMAP2(__builtin_neon_vqsubq_v, arm_neon_vqsubu, arm_neon_vqsubs),
  NEONMAP1(__builtin_neon_vraddhn_v, arm_neon_vraddhn),
  NEONMAP1(__builtin_neon_vrecps_v, arm_neon_vrecps),
  NEONMAP1(__builtin_neon_vrecpsq_v, arm_neon_vrecps),
  NEONMAP2(__builtin_neon_vrhadd_v, arm_neon_vrhaddu, arm_neon_vrhadds),
  NEONMAP2(__builtin_neon_vrhaddq_v, arm_neon_vrhaddu, arm_neon_vrhadds),
  NEONMAP2(__builtin_neon_vrshl_v, arm_neon_vrshiftu, arm_neon_vrshifts),
  NEONMAP2(__builtin_neon_vrshlq_v, arm_neon_vrshiftu, arm_neon_vrshifts),
  NEONMAP1(__builtin_neon_vrsqrts_v, arm_neon_vrsqrts),
  NEONMAP1(__builtin_neon_vrsqrtsq_v, arm_neon_vrsqrts),
  NEONMAP1(__builtin_neon_vrsubhn_v, arm_neon_vrsubhn),
  NEONMAP1(__builtin_neon_vsha1su0q_v, arm_neon_sha1su0),
  NEONMAP1(__builtin_neon_vsha1su1q_v, arm_neon_sha1su1),
  NEONMAP1(__builtin_neon_vsha256h2q_v, arm_neon_sha256h2),
  NEONMAP1(__builtin_neon_vsha256hq_v, arm_neon_sha256h),
  NEONMAP1(__builtin_neon_vsha256su0q_v, arm_neon_sha256su0),
  NEONMAP1(__builtin_neon_vsha256su1q_v, arm_neon_sha256su1),
  NEONMAP0(__builtin_neon_vshl_n_v),
  NEONMAP2(__builtin_neon_vshl_v, arm_neon_vshiftu, arm_neon_vshifts),
  NEONMAP0(__builtin_neon_vshlq_n_v),
  NEONMAP2(__builtin_neon_vshlq_v, arm_neon_vshiftu, arm_neon_vshifts),
  NEONMAP0(__builtin_neon_vshr_n_v),
  NEONMAP0(__builtin_neon_vshrn_n_v),
  NEONMAP0(__builtin_neon_vshrq_n_v),
  NEONMAP0(__builtin_neon_vsubhn_v),
  NEONMAP0(__builtin_neon_vtst_v),
  NEONMAP0(__builtin_neon_vtstq_v),
};

static CodeGenFunction::NeonIntrinsicMap ARM64NeonIntrinsicMap[] = {
  NEONMAP0(__builtin_neon_vaddhn_v),
  NEONMAP1(__builtin_neon_vaesdq_v, arm64_crypto_aesd),
  NEONMAP1(__builtin_neon_vaesdq_v, arm64_crypto_aesd),
  NEONMAP1(__builtin_neon_vaeseq_v, arm64_crypto_aese),
  NEONMAP1(__builtin_neon_vaesimcq_v, arm64_crypto_aesimc),
  NEONMAP1(__builtin_neon_vaesmcq_v, arm64_crypto_aesmc),
  NEONMAP1(__builtin_neon_vcage_v, arm64_neon_facge),
  NEONMAP1(__builtin_neon_vcageq_v, arm64_neon_facge),
  NEONMAP1(__builtin_neon_vcagt_v, arm64_neon_facgt),
  NEONMAP1(__builtin_neon_vcagtq_v, arm64_neon_facgt),
  NEONMAP1(__builtin_neon_vcale_v, arm64_neon_facge),
  NEONMAP1(__builtin_neon_vcaleq_v, arm64_neon_facge),
  NEONMAP1(__builtin_neon_vcalt_v, arm64_neon_facgt),
  NEONMAP1(__builtin_neon_vcaltq_v, arm64_neon_facgt),
  NEONMAP0(__builtin_neon_vext_v),
  NEONMAP0(__builtin_neon_vextq_v),
  NEONMAP0(__builtin_neon_vfma_v),
  NEONMAP0(__builtin_neon_vfmaq_v),
  NEONMAP2(__builtin_neon_vhadd_v, arm64_neon_uhadd, arm64_neon_shadd),
  NEONMAP2(__builtin_neon_vhaddq_v, arm64_neon_uhadd, arm64_neon_shadd),
  NEONMAP2(__builtin_neon_vhsub_v, arm64_neon_uhsub, arm64_neon_shsub),
  NEONMAP2(__builtin_neon_vhsubq_v, arm64_neon_uhsub, arm64_neon_shsub),
  NEONMAP0(__builtin_neon_vmovl_v),
  NEONMAP0(__builtin_neon_vmovn_v),
  NEONMAP1(__builtin_neon_vmul_v, arm64_neon_pmul),
  NEONMAP1(__builtin_neon_vmulq_v, arm64_neon_pmul),
  NEONMAP1(__builtin_neon_vpadd_v, arm64_neon_addp),
  NEONMAP2(__builtin_neon_vpaddl_v, arm64_neon_uaddlp, arm64_neon_saddlp),
  NEONMAP2(__builtin_neon_vpaddlq_v, arm64_neon_uaddlp, arm64_neon_saddlp),
  NEONMAP1(__builtin_neon_vpaddq_v, arm64_neon_addp),
  NEONMAP2(__builtin_neon_vqadd_v, arm64_neon_uqadd, arm64_neon_sqadd),
  NEONMAP2(__builtin_neon_vqaddq_v, arm64_neon_uqadd, arm64_neon_sqadd),
  NEONMAP1(__builtin_neon_vqdmulh_v, arm64_neon_sqdmulh),
  NEONMAP1(__builtin_neon_vqdmulhq_v, arm64_neon_sqdmulh),
  NEONMAP1(__builtin_neon_vqdmull_v, arm64_neon_sqdmull),
  NEONMAP1(__builtin_neon_vqrdmulh_v, arm64_neon_sqrdmulh),
  NEONMAP1(__builtin_neon_vqrdmulhq_v, arm64_neon_sqrdmulh),
  NEONMAP2(__builtin_neon_vqrshl_v, arm64_neon_uqrshl, arm64_neon_sqrshl),
  NEONMAP2(__builtin_neon_vqrshlq_v, arm64_neon_uqrshl, arm64_neon_sqrshl),
  NEONMAP2(__builtin_neon_vqshl_n_v, arm64_neon_uqshl, arm64_neon_sqshl),
  NEONMAP2(__builtin_neon_vqshl_v, arm64_neon_uqshl, arm64_neon_sqshl),
  NEONMAP2(__builtin_neon_vqshlq_n_v, arm64_neon_uqshl, arm64_neon_sqshl),
  NEONMAP2(__builtin_neon_vqshlq_v, arm64_neon_uqshl, arm64_neon_sqshl),
  NEONMAP2(__builtin_neon_vqsub_v, arm64_neon_uqsub, arm64_neon_sqsub),
  NEONMAP1(__builtin_neon_vraddhn_v, arm64_neon_raddhn),
  NEONMAP1(__builtin_neon_vrecps_v, arm64_neon_frecps),
  NEONMAP1(__builtin_neon_vrecpsq_v, arm64_neon_frecps),
  NEONMAP2(__builtin_neon_vrhadd_v, arm64_neon_urhadd, arm64_neon_srhadd),
  NEONMAP2(__builtin_neon_vrhaddq_v, arm64_neon_urhadd, arm64_neon_srhadd),
  NEONMAP2(__builtin_neon_vrshl_v, arm64_neon_urshl, arm64_neon_srshl),
  NEONMAP2(__builtin_neon_vrshlq_v, arm64_neon_urshl, arm64_neon_srshl),
  NEONMAP1(__builtin_neon_vrsqrts_v, arm64_neon_frsqrts),
  NEONMAP1(__builtin_neon_vrsqrtsq_v, arm64_neon_frsqrts),
  NEONMAP1(__builtin_neon_vrsubhn_v, arm64_neon_rsubhn),
  NEONMAP1(__builtin_neon_vsha1su0q_v, arm64_crypto_sha1su0),
  NEONMAP1(__builtin_neon_vsha1su1q_v, arm64_crypto_sha1su1),
  NEONMAP1(__builtin_neon_vsha256h2q_v, arm64_crypto_sha256h2),
  NEONMAP1(__builtin_neon_vsha256hq_v, arm64_crypto_sha256h),
  NEONMAP1(__builtin_neon_vsha256su0q_v, arm64_crypto_sha256su0),
  NEONMAP1(__builtin_neon_vsha256su1q_v, arm64_crypto_sha256su1),
  NEONMAP0(__builtin_neon_vshl_n_v),
  NEONMAP2(__builtin_neon_vshl_v, arm64_neon_ushl, arm64_neon_sshl),
  NEONMAP0(__builtin_neon_vshlq_n_v),
  NEONMAP2(__builtin_neon_vshlq_v, arm64_neon_ushl, arm64_neon_sshl),
  NEONMAP0(__builtin_neon_vshr_n_v),
  NEONMAP0(__builtin_neon_vshrn_n_v),
  NEONMAP0(__builtin_neon_vshrq_n_v),
  NEONMAP0(__builtin_neon_vsubhn_v),
  NEONMAP0(__builtin_neon_vtst_v),
  NEONMAP0(__builtin_neon_vtstq_v),
};

#undef NEONMAP0
#undef NEONMAP1
#undef NEONMAP2

#ifndef NDEBUG
static bool ARMIntrinsicsProvenSorted = false;
static bool ARM64IntrinsicsProvenSorted = false;
#endif

static bool findNeonIntrinsic(
    llvm::ArrayRef<CodeGenFunction::NeonIntrinsicMap> IntrinsicMap,
    unsigned BuiltinID, bool &MapProvenSorted, unsigned &LLVMIntrinsic,
    unsigned &AltLLVMIntrinsic) {
#ifndef NDEBUG
  if (!MapProvenSorted) {
    // FIXME: use std::is_sorted once C++11 is allowed
    for (unsigned i = 0; i < IntrinsicMap.size() - 1; ++i)
      assert(IntrinsicMap[i].BuiltinID <= IntrinsicMap[i + 1].BuiltinID);
    MapProvenSorted = true;
  }
#endif

  const CodeGenFunction::NeonIntrinsicMap *Builtin =
      std::lower_bound(IntrinsicMap.begin(), IntrinsicMap.end(), BuiltinID);

  if (Builtin == IntrinsicMap.end() || Builtin->BuiltinID != BuiltinID)
    return false;

  LLVMIntrinsic = Builtin->LLVMIntrinsic;
  AltLLVMIntrinsic = Builtin->AltLLVMIntrinsic;
  return true;
}


Value *CodeGenFunction::EmitAArch64BuiltinExpr(unsigned BuiltinID,
                                               const CallExpr *E) {
  // Process AArch64 scalar builtins
  if (Value *Result = EmitAArch64ScalarBuiltinExpr(*this, BuiltinID, E))
    return Result;

  // Process AArch64 table lookup builtins
  if (Value *Result = EmitAArch64TblBuiltinExpr(*this, BuiltinID, E))
    return Result;

  if (BuiltinID == AArch64::BI__clear_cache) {
    assert(E->getNumArgs() == 2 &&
           "Variadic __clear_cache slipped through on AArch64");

    const FunctionDecl *FD = E->getDirectCallee();
    SmallVector<Value *, 2> Ops;
    for (unsigned i = 0; i < E->getNumArgs(); i++)
      Ops.push_back(EmitScalarExpr(E->getArg(i)));
    llvm::Type *Ty = CGM.getTypes().ConvertType(FD->getType());
    llvm::FunctionType *FTy = cast<llvm::FunctionType>(Ty);
    StringRef Name = FD->getName();
    return EmitNounwindRuntimeCall(CGM.CreateRuntimeFunction(FTy, Name), Ops);
  }

  SmallVector<Value *, 4> Ops;
  llvm::Value *Align = 0; // Alignment for load/store

  if (BuiltinID == NEON::BI__builtin_neon_vldrq_p128) {
   Value *Op = EmitScalarExpr(E->getArg(0));
   unsigned addressSpace =
     cast<llvm::PointerType>(Op->getType())->getAddressSpace();
   llvm::Type *Ty = llvm::Type::getFP128PtrTy(getLLVMContext(), addressSpace);
   Op = Builder.CreateBitCast(Op, Ty);
   Op = Builder.CreateLoad(Op);
   Ty = llvm::Type::getIntNTy(getLLVMContext(), 128);
   return Builder.CreateBitCast(Op, Ty);
  }
  if (BuiltinID == NEON::BI__builtin_neon_vstrq_p128) {
    Value *Op0 = EmitScalarExpr(E->getArg(0));
    unsigned addressSpace =
      cast<llvm::PointerType>(Op0->getType())->getAddressSpace();
    llvm::Type *PTy = llvm::Type::getFP128PtrTy(getLLVMContext(), addressSpace);
    Op0 = Builder.CreateBitCast(Op0, PTy);
    Value *Op1 = EmitScalarExpr(E->getArg(1));
    llvm::Type *Ty = llvm::Type::getFP128Ty(getLLVMContext());
    Op1 = Builder.CreateBitCast(Op1, Ty);
    return Builder.CreateStore(Op1, Op0);
  }
  for (unsigned i = 0, e = E->getNumArgs() - 1; i != e; i++) {
    if (i == 0) {
      switch (BuiltinID) {
      case NEON::BI__builtin_neon_vld1_v:
      case NEON::BI__builtin_neon_vld1q_v:
      case NEON::BI__builtin_neon_vst1_v:
      case NEON::BI__builtin_neon_vst1q_v:
      case NEON::BI__builtin_neon_vst2_v:
      case NEON::BI__builtin_neon_vst2q_v:
      case NEON::BI__builtin_neon_vst3_v:
      case NEON::BI__builtin_neon_vst3q_v:
      case NEON::BI__builtin_neon_vst4_v:
      case NEON::BI__builtin_neon_vst4q_v:
      case NEON::BI__builtin_neon_vst1_x2_v:
      case NEON::BI__builtin_neon_vst1q_x2_v:
      case NEON::BI__builtin_neon_vst1_x3_v:
      case NEON::BI__builtin_neon_vst1q_x3_v:
      case NEON::BI__builtin_neon_vst1_x4_v:
      case NEON::BI__builtin_neon_vst1q_x4_v:
      // Handle ld1/st1 lane in this function a little different from ARM.
      case NEON::BI__builtin_neon_vld1_lane_v:
      case NEON::BI__builtin_neon_vld1q_lane_v:
      case NEON::BI__builtin_neon_vst1_lane_v:
      case NEON::BI__builtin_neon_vst1q_lane_v:
      case NEON::BI__builtin_neon_vst2_lane_v:
      case NEON::BI__builtin_neon_vst2q_lane_v:
      case NEON::BI__builtin_neon_vst3_lane_v:
      case NEON::BI__builtin_neon_vst3q_lane_v:
      case NEON::BI__builtin_neon_vst4_lane_v:
      case NEON::BI__builtin_neon_vst4q_lane_v:
      case NEON::BI__builtin_neon_vld1_dup_v:
      case NEON::BI__builtin_neon_vld1q_dup_v:
        // Get the alignment for the argument in addition to the value;
        // we'll use it later.
        std::pair<llvm::Value *, unsigned> Src =
            EmitPointerWithAlignment(E->getArg(0));
        Ops.push_back(Src.first);
        Align = Builder.getInt32(Src.second);
        continue;
      }
    }
    if (i == 1) {
      switch (BuiltinID) {
      case NEON::BI__builtin_neon_vld2_v:
      case NEON::BI__builtin_neon_vld2q_v:
      case NEON::BI__builtin_neon_vld3_v:
      case NEON::BI__builtin_neon_vld3q_v:
      case NEON::BI__builtin_neon_vld4_v:
      case NEON::BI__builtin_neon_vld4q_v:
      case NEON::BI__builtin_neon_vld1_x2_v:
      case NEON::BI__builtin_neon_vld1q_x2_v:
      case NEON::BI__builtin_neon_vld1_x3_v:
      case NEON::BI__builtin_neon_vld1q_x3_v:
      case NEON::BI__builtin_neon_vld1_x4_v:
      case NEON::BI__builtin_neon_vld1q_x4_v:
      // Handle ld1/st1 dup lane in this function a little different from ARM.
      case NEON::BI__builtin_neon_vld2_dup_v:
      case NEON::BI__builtin_neon_vld2q_dup_v:
      case NEON::BI__builtin_neon_vld3_dup_v:
      case NEON::BI__builtin_neon_vld3q_dup_v:
      case NEON::BI__builtin_neon_vld4_dup_v:
      case NEON::BI__builtin_neon_vld4q_dup_v:
      case NEON::BI__builtin_neon_vld2_lane_v:
      case NEON::BI__builtin_neon_vld2q_lane_v:
      case NEON::BI__builtin_neon_vld3_lane_v:
      case NEON::BI__builtin_neon_vld3q_lane_v:
      case NEON::BI__builtin_neon_vld4_lane_v:
      case NEON::BI__builtin_neon_vld4q_lane_v:
        // Get the alignment for the argument in addition to the value;
        // we'll use it later.
        std::pair<llvm::Value *, unsigned> Src =
            EmitPointerWithAlignment(E->getArg(1));
        Ops.push_back(Src.first);
        Align = Builder.getInt32(Src.second);
        continue;
      }
    }
    Ops.push_back(EmitScalarExpr(E->getArg(i)));
  }

  // Get the last argument, which specifies the vector type.
  llvm::APSInt Result;
  const Expr *Arg = E->getArg(E->getNumArgs() - 1);
  if (!Arg->isIntegerConstantExpr(Result, getContext()))
    return 0;

  // Determine the type of this overloaded NEON intrinsic.
  NeonTypeFlags Type(Result.getZExtValue());
  bool usgn = Type.isUnsigned();
  bool quad = Type.isQuad();

  llvm::VectorType *VTy = GetNeonType(this, Type);
  llvm::Type *Ty = VTy;
  if (!Ty)
    return 0;


  // Many NEON builtins have identical semantics and uses in ARM and
  // AArch64. Emit these in a single function.
  unsigned LLVMIntrinsic = 0, AltLLVMIntrinsic = 0;
  findNeonIntrinsic(ARMNeonIntrinsicMap, BuiltinID, ARMIntrinsicsProvenSorted,
                    LLVMIntrinsic, AltLLVMIntrinsic);

  if (Value *Result = EmitCommonNeonBuiltinExpr(
          BuiltinID, LLVMIntrinsic, AltLLVMIntrinsic, E, Ops, Align))
    return Result;

  unsigned Int;
  switch (BuiltinID) {
  default:
    return 0;

  // AArch64 builtins mapping to legacy ARM v7 builtins.
  // FIXME: the mapped builtins listed correspond to what has been tested
  // in aarch64-neon-intrinsics.c so far.

  // Shift by immediate
  case NEON::BI__builtin_neon_vrshr_n_v:
  case NEON::BI__builtin_neon_vrshrq_n_v:
    Int = usgn ? Intrinsic::aarch64_neon_vurshr
               : Intrinsic::aarch64_neon_vsrshr;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrshr_n");
  case NEON::BI__builtin_neon_vsra_n_v:
    if (VTy->getElementType()->isIntegerTy(64)) {
      Int = usgn ? Intrinsic::aarch64_neon_vsradu_n
                 : Intrinsic::aarch64_neon_vsrads_n;
      return EmitNeonCall(CGM.getIntrinsic(Int), Ops, "vsra_n");
    }
    return EmitARMBuiltinExpr(NEON::BI__builtin_neon_vsra_n_v, E);
  case NEON::BI__builtin_neon_vsraq_n_v:
    return EmitARMBuiltinExpr(NEON::BI__builtin_neon_vsraq_n_v, E);
  case NEON::BI__builtin_neon_vrsra_n_v:
    if (VTy->getElementType()->isIntegerTy(64)) {
      Int = usgn ? Intrinsic::aarch64_neon_vrsradu_n
                 : Intrinsic::aarch64_neon_vrsrads_n;
      return EmitNeonCall(CGM.getIntrinsic(Int), Ops, "vrsra_n");
    }
    // fall through
  case NEON::BI__builtin_neon_vrsraq_n_v: {
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Int = usgn ? Intrinsic::aarch64_neon_vurshr
               : Intrinsic::aarch64_neon_vsrshr;
    Ops[1] = Builder.CreateCall2(CGM.getIntrinsic(Int, Ty), Ops[1], Ops[2]);
    return Builder.CreateAdd(Ops[0], Ops[1], "vrsra_n");
  }
  case NEON::BI__builtin_neon_vqshlu_n_v:
  case NEON::BI__builtin_neon_vqshluq_n_v:
    Int = Intrinsic::aarch64_neon_vsqshlu;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqshlu_n");
  case NEON::BI__builtin_neon_vsri_n_v:
  case NEON::BI__builtin_neon_vsriq_n_v:
    Int = Intrinsic::aarch64_neon_vsri;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vsri_n");
  case NEON::BI__builtin_neon_vsli_n_v:
  case NEON::BI__builtin_neon_vsliq_n_v:
    Int = Intrinsic::aarch64_neon_vsli;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vsli_n");
  case NEON::BI__builtin_neon_vshll_n_v: {
    llvm::Type *SrcTy = llvm::VectorType::getTruncatedElementVectorType(VTy);
    Ops[0] = Builder.CreateBitCast(Ops[0], SrcTy);
    if (usgn)
      Ops[0] = Builder.CreateZExt(Ops[0], VTy);
    else
      Ops[0] = Builder.CreateSExt(Ops[0], VTy);
    Ops[1] = EmitNeonShiftVector(Ops[1], VTy, false);
    return Builder.CreateShl(Ops[0], Ops[1], "vshll_n");
  }
  case NEON::BI__builtin_neon_vqshrun_n_v:
    Int = Intrinsic::aarch64_neon_vsqshrun;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqshrun_n");
  case NEON::BI__builtin_neon_vrshrn_n_v:
    Int = Intrinsic::aarch64_neon_vrshrn;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrshrn_n");
  case NEON::BI__builtin_neon_vqrshrun_n_v:
    Int = Intrinsic::aarch64_neon_vsqrshrun;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqrshrun_n");
  case NEON::BI__builtin_neon_vqshrn_n_v:
    Int = usgn ? Intrinsic::aarch64_neon_vuqshrn
               : Intrinsic::aarch64_neon_vsqshrn;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqshrn_n");
  case NEON::BI__builtin_neon_vqrshrn_n_v:
    Int = usgn ? Intrinsic::aarch64_neon_vuqrshrn
               : Intrinsic::aarch64_neon_vsqrshrn;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqrshrn_n");

  // Convert
  case NEON::BI__builtin_neon_vcvt_n_f64_v:
  case NEON::BI__builtin_neon_vcvtq_n_f64_v: {
    llvm::Type *FloatTy =
        GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float64, false, quad));
    llvm::Type *Tys[2] = { FloatTy, Ty };
    Int = usgn ? Intrinsic::arm_neon_vcvtfxu2fp
               : Intrinsic::arm_neon_vcvtfxs2fp;
    Function *F = CGM.getIntrinsic(Int, Tys);
    return EmitNeonCall(F, Ops, "vcvt_n");
  }
  case NEON::BI__builtin_neon_vcvt_n_s64_v:
  case NEON::BI__builtin_neon_vcvt_n_u64_v:
  case NEON::BI__builtin_neon_vcvtq_n_s64_v:
  case NEON::BI__builtin_neon_vcvtq_n_u64_v: {
    llvm::Type *FloatTy =
        GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float64, false, quad));
    llvm::Type *Tys[2] = { Ty, FloatTy };
    Int = usgn ? Intrinsic::arm_neon_vcvtfp2fxu
               : Intrinsic::arm_neon_vcvtfp2fxs;
    Function *F = CGM.getIntrinsic(Int, Tys);
    return EmitNeonCall(F, Ops, "vcvt_n");
  }

  // Load/Store
  case NEON::BI__builtin_neon_vld1_x2_v:
  case NEON::BI__builtin_neon_vld1q_x2_v:
  case NEON::BI__builtin_neon_vld1_x3_v:
  case NEON::BI__builtin_neon_vld1q_x3_v:
  case NEON::BI__builtin_neon_vld1_x4_v:
  case NEON::BI__builtin_neon_vld1q_x4_v: {
    unsigned Int;
    switch (BuiltinID) {
    case NEON::BI__builtin_neon_vld1_x2_v:
    case NEON::BI__builtin_neon_vld1q_x2_v:
      Int = Intrinsic::aarch64_neon_vld1x2;
      break;
    case NEON::BI__builtin_neon_vld1_x3_v:
    case NEON::BI__builtin_neon_vld1q_x3_v:
      Int = Intrinsic::aarch64_neon_vld1x3;
      break;
    case NEON::BI__builtin_neon_vld1_x4_v:
    case NEON::BI__builtin_neon_vld1q_x4_v:
      Int = Intrinsic::aarch64_neon_vld1x4;
      break;
    }
    Function *F = CGM.getIntrinsic(Int, Ty);
    Ops[1] = Builder.CreateCall2(F, Ops[1], Align, "vld1xN");
    Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vst1_x2_v:
  case NEON::BI__builtin_neon_vst1q_x2_v:
  case NEON::BI__builtin_neon_vst1_x3_v:
  case NEON::BI__builtin_neon_vst1q_x3_v:
  case NEON::BI__builtin_neon_vst1_x4_v:
  case NEON::BI__builtin_neon_vst1q_x4_v: {
    Ops.push_back(Align);
    unsigned Int;
    switch (BuiltinID) {
    case NEON::BI__builtin_neon_vst1_x2_v:
    case NEON::BI__builtin_neon_vst1q_x2_v:
      Int = Intrinsic::aarch64_neon_vst1x2;
      break;
    case NEON::BI__builtin_neon_vst1_x3_v:
    case NEON::BI__builtin_neon_vst1q_x3_v:
      Int = Intrinsic::aarch64_neon_vst1x3;
      break;
    case NEON::BI__builtin_neon_vst1_x4_v:
    case NEON::BI__builtin_neon_vst1q_x4_v:
      Int = Intrinsic::aarch64_neon_vst1x4;
      break;
    }
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "");
  }
  case NEON::BI__builtin_neon_vld1_lane_v:
  case NEON::BI__builtin_neon_vld1q_lane_v: {
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ty = llvm::PointerType::getUnqual(VTy->getElementType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    LoadInst *Ld = Builder.CreateLoad(Ops[0]);
    Ld->setAlignment(cast<ConstantInt>(Align)->getZExtValue());
    return Builder.CreateInsertElement(Ops[1], Ld, Ops[2], "vld1_lane");
  }
  case NEON::BI__builtin_neon_vst1_lane_v:
  case NEON::BI__builtin_neon_vst1q_lane_v: {
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[1] = Builder.CreateExtractElement(Ops[1], Ops[2]);
    Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
    StoreInst *St =
        Builder.CreateStore(Ops[1], Builder.CreateBitCast(Ops[0], Ty));
    St->setAlignment(cast<ConstantInt>(Align)->getZExtValue());
    return St;
  }
  case NEON::BI__builtin_neon_vld2_dup_v:
  case NEON::BI__builtin_neon_vld2q_dup_v:
  case NEON::BI__builtin_neon_vld3_dup_v:
  case NEON::BI__builtin_neon_vld3q_dup_v:
  case NEON::BI__builtin_neon_vld4_dup_v:
  case NEON::BI__builtin_neon_vld4q_dup_v: {
    // Handle 64-bit x 1 elements as a special-case.  There is no "dup" needed.
    if (VTy->getElementType()->getPrimitiveSizeInBits() == 64 &&
        VTy->getNumElements() == 1) {
      switch (BuiltinID) {
      case NEON::BI__builtin_neon_vld2_dup_v:
        Int = Intrinsic::arm_neon_vld2;
        break;
      case NEON::BI__builtin_neon_vld3_dup_v:
        Int = Intrinsic::arm_neon_vld3;
        break;
      case NEON::BI__builtin_neon_vld4_dup_v:
        Int = Intrinsic::arm_neon_vld4;
        break;
      default:
        llvm_unreachable("unknown vld_dup intrinsic?");
      }
      Function *F = CGM.getIntrinsic(Int, Ty);
      Ops[1] = Builder.CreateCall2(F, Ops[1], Align, "vld_dup");
      Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
      Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
      return Builder.CreateStore(Ops[1], Ops[0]);
    }
    switch (BuiltinID) {
    case NEON::BI__builtin_neon_vld2_dup_v:
    case NEON::BI__builtin_neon_vld2q_dup_v:
      Int = Intrinsic::arm_neon_vld2lane;
      break;
    case NEON::BI__builtin_neon_vld3_dup_v:
    case NEON::BI__builtin_neon_vld3q_dup_v:
      Int = Intrinsic::arm_neon_vld3lane;
      break;
    case NEON::BI__builtin_neon_vld4_dup_v:
    case NEON::BI__builtin_neon_vld4q_dup_v:
      Int = Intrinsic::arm_neon_vld4lane;
      break;
    }
    Function *F = CGM.getIntrinsic(Int, Ty);
    llvm::StructType *STy = cast<llvm::StructType>(F->getReturnType());

    SmallVector<Value *, 6> Args;
    Args.push_back(Ops[1]);
    Args.append(STy->getNumElements(), UndefValue::get(Ty));

    llvm::Constant *CI = ConstantInt::get(Int32Ty, 0);
    Args.push_back(CI);
    Args.push_back(Align);

    Ops[1] = Builder.CreateCall(F, Args, "vld_dup");
    // splat lane 0 to all elts in each vector of the result.
    for (unsigned i = 0, e = STy->getNumElements(); i != e; ++i) {
      Value *Val = Builder.CreateExtractValue(Ops[1], i);
      Value *Elt = Builder.CreateBitCast(Val, Ty);
      Elt = EmitNeonSplat(Elt, CI);
      Elt = Builder.CreateBitCast(Elt, Val->getType());
      Ops[1] = Builder.CreateInsertValue(Ops[1], Elt, i);
    }
    Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    return Builder.CreateStore(Ops[1], Ops[0]);
  }

  case NEON::BI__builtin_neon_vmul_lane_v:
  case NEON::BI__builtin_neon_vmul_laneq_v: {
    // v1f64 vmul_lane should be mapped to Neon scalar mul lane
    bool Quad = false;
    if (BuiltinID == NEON::BI__builtin_neon_vmul_laneq_v)
      Quad = true;
    Ops[0] = Builder.CreateBitCast(Ops[0], DoubleTy);
    llvm::Type *VTy = GetNeonType(this,
      NeonTypeFlags(NeonTypeFlags::Float64, false, Quad));
    Ops[1] = Builder.CreateBitCast(Ops[1], VTy);
    Ops[1] = Builder.CreateExtractElement(Ops[1], Ops[2], "extract");
    Value *Result = Builder.CreateFMul(Ops[0], Ops[1]);
    return Builder.CreateBitCast(Result, Ty);
  }

  // AArch64-only builtins
  case NEON::BI__builtin_neon_vfmaq_laneq_v: {
    Value *F = CGM.getIntrinsic(Intrinsic::fma, Ty);
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);

    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);
    Ops[2] = EmitNeonSplat(Ops[2], cast<ConstantInt>(Ops[3]));
    return Builder.CreateCall3(F, Ops[2], Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vfmaq_lane_v: {
    Value *F = CGM.getIntrinsic(Intrinsic::fma, Ty);
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);

    llvm::VectorType *VTy = cast<llvm::VectorType>(Ty);
    llvm::Type *STy = llvm::VectorType::get(VTy->getElementType(),
                                            VTy->getNumElements() / 2);
    Ops[2] = Builder.CreateBitCast(Ops[2], STy);
    Value* SV = llvm::ConstantVector::getSplat(VTy->getNumElements(),
                                               cast<ConstantInt>(Ops[3]));
    Ops[2] = Builder.CreateShuffleVector(Ops[2], Ops[2], SV, "lane");

    return Builder.CreateCall3(F, Ops[2], Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vfma_lane_v: {
    llvm::VectorType *VTy = cast<llvm::VectorType>(Ty);
    // v1f64 fma should be mapped to Neon scalar f64 fma
    if (VTy && VTy->getElementType() == DoubleTy) {
      Ops[0] = Builder.CreateBitCast(Ops[0], DoubleTy);
      Ops[1] = Builder.CreateBitCast(Ops[1], DoubleTy);
      llvm::Type *VTy = GetNeonType(this,
        NeonTypeFlags(NeonTypeFlags::Float64, false, false));
      Ops[2] = Builder.CreateBitCast(Ops[2], VTy);
      Ops[2] = Builder.CreateExtractElement(Ops[2], Ops[3], "extract");
      Value *F = CGM.getIntrinsic(Intrinsic::fma, DoubleTy);
      Value *Result = Builder.CreateCall3(F, Ops[1], Ops[2], Ops[0]);
      return Builder.CreateBitCast(Result, Ty);
    }
    Value *F = CGM.getIntrinsic(Intrinsic::fma, Ty);
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);

    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);
    Ops[2] = EmitNeonSplat(Ops[2], cast<ConstantInt>(Ops[3]));
    return Builder.CreateCall3(F, Ops[2], Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vfma_laneq_v: {
    llvm::VectorType *VTy = cast<llvm::VectorType>(Ty);
    // v1f64 fma should be mapped to Neon scalar f64 fma
    if (VTy && VTy->getElementType() == DoubleTy) {
      Ops[0] = Builder.CreateBitCast(Ops[0], DoubleTy);
      Ops[1] = Builder.CreateBitCast(Ops[1], DoubleTy);
      llvm::Type *VTy = GetNeonType(this,
        NeonTypeFlags(NeonTypeFlags::Float64, false, true));
      Ops[2] = Builder.CreateBitCast(Ops[2], VTy);
      Ops[2] = Builder.CreateExtractElement(Ops[2], Ops[3], "extract");
      Value *F = CGM.getIntrinsic(Intrinsic::fma, DoubleTy);
      Value *Result = Builder.CreateCall3(F, Ops[1], Ops[2], Ops[0]);
      return Builder.CreateBitCast(Result, Ty);
    }
    Value *F = CGM.getIntrinsic(Intrinsic::fma, Ty);
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);

    llvm::Type *STy = llvm::VectorType::get(VTy->getElementType(),
                                            VTy->getNumElements() * 2);
    Ops[2] = Builder.CreateBitCast(Ops[2], STy);
    Value* SV = llvm::ConstantVector::getSplat(VTy->getNumElements(),
                                               cast<ConstantInt>(Ops[3]));
    Ops[2] = Builder.CreateShuffleVector(Ops[2], Ops[2], SV, "lane");

    return Builder.CreateCall3(F, Ops[2], Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vfms_v:
  case NEON::BI__builtin_neon_vfmsq_v: {
    Value *F = CGM.getIntrinsic(Intrinsic::fma, Ty);
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[1] = Builder.CreateFNeg(Ops[1]);
    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);

    // LLVM's fma intrinsic puts the accumulator in the last position, but the
    // AArch64 intrinsic has it first.
    return Builder.CreateCall3(F, Ops[1], Ops[2], Ops[0]);
  }
  case NEON::BI__builtin_neon_vmaxnm_v:
  case NEON::BI__builtin_neon_vmaxnmq_v: {
    Int = Intrinsic::aarch64_neon_vmaxnm;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vmaxnm");
  }
  case NEON::BI__builtin_neon_vminnm_v:
  case NEON::BI__builtin_neon_vminnmq_v: {
    Int = Intrinsic::aarch64_neon_vminnm;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vminnm");
  }
  case NEON::BI__builtin_neon_vpmaxnm_v:
  case NEON::BI__builtin_neon_vpmaxnmq_v: {
    Int = Intrinsic::aarch64_neon_vpmaxnm;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vpmaxnm");
  }
  case NEON::BI__builtin_neon_vpminnm_v:
  case NEON::BI__builtin_neon_vpminnmq_v: {
    Int = Intrinsic::aarch64_neon_vpminnm;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vpminnm");
  }
  case NEON::BI__builtin_neon_vpmaxq_v: {
    Int = usgn ? Intrinsic::arm_neon_vpmaxu : Intrinsic::arm_neon_vpmaxs;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vpmax");
  }
  case NEON::BI__builtin_neon_vpminq_v: {
    Int = usgn ? Intrinsic::arm_neon_vpminu : Intrinsic::arm_neon_vpmins;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vpmin");
  }
  case NEON::BI__builtin_neon_vpaddq_v: {
    Int = Intrinsic::arm_neon_vpadd;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vpadd");
  }
  case NEON::BI__builtin_neon_vmulx_v:
  case NEON::BI__builtin_neon_vmulxq_v: {
    Int = Intrinsic::aarch64_neon_vmulx;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vmulx");
  }
  case NEON::BI__builtin_neon_vabs_v:
  case NEON::BI__builtin_neon_vabsq_v: {
    if (VTy->getElementType()->isFloatingPointTy()) {
      return EmitNeonCall(CGM.getIntrinsic(Intrinsic::fabs, Ty), Ops, "vabs");
    }
    return EmitARMBuiltinExpr(NEON::BI__builtin_neon_vabs_v, E);
  }
  case NEON::BI__builtin_neon_vsqadd_v:
  case NEON::BI__builtin_neon_vsqaddq_v: {
    Int = Intrinsic::aarch64_neon_usqadd;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vsqadd");
  }
  case NEON::BI__builtin_neon_vuqadd_v:
  case NEON::BI__builtin_neon_vuqaddq_v: {
    Int = Intrinsic::aarch64_neon_suqadd;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vuqadd");
  }
  case NEON::BI__builtin_neon_vrbit_v:
  case NEON::BI__builtin_neon_vrbitq_v:
    Int = Intrinsic::aarch64_neon_rbit;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrbit");
  case NEON::BI__builtin_neon_vcvt_f32_f64: {
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ty = GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float32, false, false));
    return Builder.CreateFPTrunc(Ops[0], Ty, "vcvt");
  }
  case NEON::BI__builtin_neon_vcvtx_f32_v: {
    llvm::Type *EltTy = FloatTy;
    llvm::Type *ResTy = llvm::VectorType::get(EltTy, 2);
    llvm::Type *Tys[2] = { ResTy, Ty };
    Int = Intrinsic::aarch64_neon_vcvtxn;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtx_f32_f64");
  }
  case NEON::BI__builtin_neon_vcvt_f64_f32: {
    llvm::Type *OpTy =
        GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float32, false, false));
    Ops[0] = Builder.CreateBitCast(Ops[0], OpTy);
    return Builder.CreateFPExt(Ops[0], Ty, "vcvt");
  }
  case NEON::BI__builtin_neon_vcvt_f64_v:
  case NEON::BI__builtin_neon_vcvtq_f64_v: {
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ty = GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float64, false, quad));
    return usgn ? Builder.CreateUIToFP(Ops[0], Ty, "vcvt")
                : Builder.CreateSIToFP(Ops[0], Ty, "vcvt");
  }
  case NEON::BI__builtin_neon_vrndn_v:
  case NEON::BI__builtin_neon_vrndnq_v: {
    Int = Intrinsic::aarch64_neon_frintn;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrndn");
  }
  case NEON::BI__builtin_neon_vrnda_v:
  case NEON::BI__builtin_neon_vrndaq_v: {
    Int = Intrinsic::round;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrnda");
  }
  case NEON::BI__builtin_neon_vrndp_v:
  case NEON::BI__builtin_neon_vrndpq_v: {
    Int = Intrinsic::ceil;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrndp");
  }
  case NEON::BI__builtin_neon_vrndm_v:
  case NEON::BI__builtin_neon_vrndmq_v: {
    Int = Intrinsic::floor;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrndm");
  }
  case NEON::BI__builtin_neon_vrndx_v:
  case NEON::BI__builtin_neon_vrndxq_v: {
    Int = Intrinsic::rint;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrndx");
  }
  case NEON::BI__builtin_neon_vrnd_v:
  case NEON::BI__builtin_neon_vrndq_v: {
    Int = Intrinsic::trunc;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrnd");
  }
  case NEON::BI__builtin_neon_vrndi_v:
  case NEON::BI__builtin_neon_vrndiq_v: {
    Int = Intrinsic::nearbyint;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrndi");
  }
  case NEON::BI__builtin_neon_vcvt_s64_v:
  case NEON::BI__builtin_neon_vcvt_u64_v:
  case NEON::BI__builtin_neon_vcvtq_s64_v:
  case NEON::BI__builtin_neon_vcvtq_u64_v: {
    llvm::Type *DoubleTy =
        GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float64, false, quad));
    Ops[0] = Builder.CreateBitCast(Ops[0], DoubleTy);
    return usgn ? Builder.CreateFPToUI(Ops[0], Ty, "vcvt")
                : Builder.CreateFPToSI(Ops[0], Ty, "vcvt");
  }
  case NEON::BI__builtin_neon_vcvtn_s32_v:
  case NEON::BI__builtin_neon_vcvtnq_s32_v: {
    llvm::Type *OpTy = llvm::VectorType::get(FloatTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtns;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtns_f32");
  }
  case NEON::BI__builtin_neon_vcvtn_s64_v:
  case NEON::BI__builtin_neon_vcvtnq_s64_v: {
    llvm::Type *OpTy = llvm::VectorType::get(DoubleTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtns;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtns_f64");
  }
  case NEON::BI__builtin_neon_vcvtn_u32_v:
  case NEON::BI__builtin_neon_vcvtnq_u32_v: {
    llvm::Type *OpTy = llvm::VectorType::get(FloatTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtnu;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtnu_f32");
  }
  case NEON::BI__builtin_neon_vcvtn_u64_v:
  case NEON::BI__builtin_neon_vcvtnq_u64_v: {
    llvm::Type *OpTy = llvm::VectorType::get(DoubleTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtnu;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtnu_f64");
  }
  case NEON::BI__builtin_neon_vcvtp_s32_v:
  case NEON::BI__builtin_neon_vcvtpq_s32_v: {
    llvm::Type *OpTy = llvm::VectorType::get(FloatTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtps;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtps_f32");
  }
  case NEON::BI__builtin_neon_vcvtp_s64_v:
  case NEON::BI__builtin_neon_vcvtpq_s64_v: {
    llvm::Type *OpTy = llvm::VectorType::get(DoubleTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtps;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtps_f64");
  }
  case NEON::BI__builtin_neon_vcvtp_u32_v:
  case NEON::BI__builtin_neon_vcvtpq_u32_v: {
    llvm::Type *OpTy = llvm::VectorType::get(FloatTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtpu;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtpu_f32");
  }
  case NEON::BI__builtin_neon_vcvtp_u64_v:
  case NEON::BI__builtin_neon_vcvtpq_u64_v: {
    llvm::Type *OpTy = llvm::VectorType::get(DoubleTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtpu;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtpu_f64");
  }
  case NEON::BI__builtin_neon_vcvtm_s32_v:
  case NEON::BI__builtin_neon_vcvtmq_s32_v: {
    llvm::Type *OpTy = llvm::VectorType::get(FloatTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtms;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtms_f32");
  }
  case NEON::BI__builtin_neon_vcvtm_s64_v:
  case NEON::BI__builtin_neon_vcvtmq_s64_v: {
    llvm::Type *OpTy = llvm::VectorType::get(DoubleTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtms;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtms_f64");
  }
  case NEON::BI__builtin_neon_vcvtm_u32_v:
  case NEON::BI__builtin_neon_vcvtmq_u32_v: {
    llvm::Type *OpTy = llvm::VectorType::get(FloatTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtmu;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtmu_f32");
  }
  case NEON::BI__builtin_neon_vcvtm_u64_v:
  case NEON::BI__builtin_neon_vcvtmq_u64_v: {
    llvm::Type *OpTy = llvm::VectorType::get(DoubleTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtmu;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtmu_f64");
  }
  case NEON::BI__builtin_neon_vcvta_s32_v:
  case NEON::BI__builtin_neon_vcvtaq_s32_v: {
    llvm::Type *OpTy = llvm::VectorType::get(FloatTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtas;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtas_f32");
  }
  case NEON::BI__builtin_neon_vcvta_s64_v:
  case NEON::BI__builtin_neon_vcvtaq_s64_v: {
    llvm::Type *OpTy = llvm::VectorType::get(DoubleTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtas;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtas_f64");
  }
  case NEON::BI__builtin_neon_vcvta_u32_v:
  case NEON::BI__builtin_neon_vcvtaq_u32_v: {
    llvm::Type *OpTy = llvm::VectorType::get(FloatTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtau;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtau_f32");
  }
  case NEON::BI__builtin_neon_vcvta_u64_v:
  case NEON::BI__builtin_neon_vcvtaq_u64_v: {
    llvm::Type *OpTy = llvm::VectorType::get(DoubleTy, VTy->getNumElements());
    llvm::Type *Tys[2] = { Ty, OpTy };
    Int = Intrinsic::arm_neon_vcvtau;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtau_f64");
  }
  case NEON::BI__builtin_neon_vsqrt_v:
  case NEON::BI__builtin_neon_vsqrtq_v: {
    Int = Intrinsic::sqrt;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vsqrt");
  }
  case NEON::BI__builtin_neon_vceqz_v:
  case NEON::BI__builtin_neon_vceqzq_v:
    return EmitAArch64CompareBuiltinExpr(Ops[0], Ty, ICmpInst::FCMP_OEQ,
                                         ICmpInst::ICMP_EQ, "vceqz");
  case NEON::BI__builtin_neon_vcgez_v:
  case NEON::BI__builtin_neon_vcgezq_v:
    return EmitAArch64CompareBuiltinExpr(Ops[0], Ty, ICmpInst::FCMP_OGE,
                                         ICmpInst::ICMP_SGE, "vcgez");
  case NEON::BI__builtin_neon_vclez_v:
  case NEON::BI__builtin_neon_vclezq_v:
    return EmitAArch64CompareBuiltinExpr(Ops[0], Ty, ICmpInst::FCMP_OLE,
                                         ICmpInst::ICMP_SLE, "vclez");
  case NEON::BI__builtin_neon_vcgtz_v:
  case NEON::BI__builtin_neon_vcgtzq_v:
    return EmitAArch64CompareBuiltinExpr(Ops[0], Ty, ICmpInst::FCMP_OGT,
                                         ICmpInst::ICMP_SGT, "vcgtz");
  case NEON::BI__builtin_neon_vcltz_v:
  case NEON::BI__builtin_neon_vcltzq_v:
    return EmitAArch64CompareBuiltinExpr(Ops[0], Ty, ICmpInst::FCMP_OLT,
                                         ICmpInst::ICMP_SLT, "vcltz");
  }
}

Value *CodeGenFunction::EmitARMBuiltinExpr(unsigned BuiltinID,
                                           const CallExpr *E) {
  if (BuiltinID == ARM::BI__clear_cache) {
    assert(E->getNumArgs() == 2 && "__clear_cache takes 2 arguments");
    const FunctionDecl *FD = E->getDirectCallee();
    SmallVector<Value*, 2> Ops;
    for (unsigned i = 0; i < 2; i++)
      Ops.push_back(EmitScalarExpr(E->getArg(i)));
    llvm::Type *Ty = CGM.getTypes().ConvertType(FD->getType());
    llvm::FunctionType *FTy = cast<llvm::FunctionType>(Ty);
    StringRef Name = FD->getName();
    return EmitNounwindRuntimeCall(CGM.CreateRuntimeFunction(FTy, Name), Ops);
  }

  if (BuiltinID == ARM::BI__builtin_arm_ldrexd ||
      (BuiltinID == ARM::BI__builtin_arm_ldrex &&
       getContext().getTypeSize(E->getType()) == 64)) {
    Function *F = CGM.getIntrinsic(Intrinsic::arm_ldrexd);

    Value *LdPtr = EmitScalarExpr(E->getArg(0));
    Value *Val = Builder.CreateCall(F, Builder.CreateBitCast(LdPtr, Int8PtrTy),
                                    "ldrexd");

    Value *Val0 = Builder.CreateExtractValue(Val, 1);
    Value *Val1 = Builder.CreateExtractValue(Val, 0);
    Val0 = Builder.CreateZExt(Val0, Int64Ty);
    Val1 = Builder.CreateZExt(Val1, Int64Ty);

    Value *ShiftCst = llvm::ConstantInt::get(Int64Ty, 32);
    Val = Builder.CreateShl(Val0, ShiftCst, "shl", true /* nuw */);
    Val = Builder.CreateOr(Val, Val1);
    return Builder.CreateBitCast(Val, ConvertType(E->getType()));
  }

  if (BuiltinID == ARM::BI__builtin_arm_ldrex) {
    Value *LoadAddr = EmitScalarExpr(E->getArg(0));

    QualType Ty = E->getType();
    llvm::Type *RealResTy = ConvertType(Ty);
    llvm::Type *IntResTy = llvm::IntegerType::get(getLLVMContext(),
                                                  getContext().getTypeSize(Ty));
    LoadAddr = Builder.CreateBitCast(LoadAddr, IntResTy->getPointerTo());

    Function *F = CGM.getIntrinsic(Intrinsic::arm_ldrex, LoadAddr->getType());
    Value *Val = Builder.CreateCall(F, LoadAddr, "ldrex");

    if (RealResTy->isPointerTy())
      return Builder.CreateIntToPtr(Val, RealResTy);
    else {
      Val = Builder.CreateTruncOrBitCast(Val, IntResTy);
      return Builder.CreateBitCast(Val, RealResTy);
    }
  }

  if (BuiltinID == ARM::BI__builtin_arm_strexd ||
      (BuiltinID == ARM::BI__builtin_arm_strex &&
       getContext().getTypeSize(E->getArg(0)->getType()) == 64)) {
    Function *F = CGM.getIntrinsic(Intrinsic::arm_strexd);
    llvm::Type *STy = llvm::StructType::get(Int32Ty, Int32Ty, NULL);

    Value *Tmp = CreateMemTemp(E->getArg(0)->getType());
    Value *Val = EmitScalarExpr(E->getArg(0));
    Builder.CreateStore(Val, Tmp);

    Value *LdPtr = Builder.CreateBitCast(Tmp,llvm::PointerType::getUnqual(STy));
    Val = Builder.CreateLoad(LdPtr);

    Value *Arg0 = Builder.CreateExtractValue(Val, 0);
    Value *Arg1 = Builder.CreateExtractValue(Val, 1);
    Value *StPtr = Builder.CreateBitCast(EmitScalarExpr(E->getArg(1)), Int8PtrTy);
    return Builder.CreateCall3(F, Arg0, Arg1, StPtr, "strexd");
  }

  if (BuiltinID == ARM::BI__builtin_arm_strex) {
    Value *StoreVal = EmitScalarExpr(E->getArg(0));
    Value *StoreAddr = EmitScalarExpr(E->getArg(1));

    QualType Ty = E->getArg(0)->getType();
    llvm::Type *StoreTy = llvm::IntegerType::get(getLLVMContext(),
                                                 getContext().getTypeSize(Ty));
    StoreAddr = Builder.CreateBitCast(StoreAddr, StoreTy->getPointerTo());

    if (StoreVal->getType()->isPointerTy())
      StoreVal = Builder.CreatePtrToInt(StoreVal, Int32Ty);
    else {
      StoreVal = Builder.CreateBitCast(StoreVal, StoreTy);
      StoreVal = Builder.CreateZExtOrBitCast(StoreVal, Int32Ty);
    }

    Function *F = CGM.getIntrinsic(Intrinsic::arm_strex, StoreAddr->getType());
    return Builder.CreateCall2(F, StoreVal, StoreAddr, "strex");
  }

  if (BuiltinID == ARM::BI__builtin_arm_clrex) {
    Function *F = CGM.getIntrinsic(Intrinsic::arm_clrex);
    return Builder.CreateCall(F);
  }

  if (BuiltinID == ARM::BI__builtin_arm_sevl) {
    Function *F = CGM.getIntrinsic(Intrinsic::arm_sevl);
    return Builder.CreateCall(F);
  }

  // CRC32
  Intrinsic::ID CRCIntrinsicID = Intrinsic::not_intrinsic;
  switch (BuiltinID) {
  case ARM::BI__builtin_arm_crc32b:
    CRCIntrinsicID = Intrinsic::arm_crc32b; break;
  case ARM::BI__builtin_arm_crc32cb:
    CRCIntrinsicID = Intrinsic::arm_crc32cb; break;
  case ARM::BI__builtin_arm_crc32h:
    CRCIntrinsicID = Intrinsic::arm_crc32h; break;
  case ARM::BI__builtin_arm_crc32ch:
    CRCIntrinsicID = Intrinsic::arm_crc32ch; break;
  case ARM::BI__builtin_arm_crc32w:
  case ARM::BI__builtin_arm_crc32d:
    CRCIntrinsicID = Intrinsic::arm_crc32w; break;
  case ARM::BI__builtin_arm_crc32cw:
  case ARM::BI__builtin_arm_crc32cd:
    CRCIntrinsicID = Intrinsic::arm_crc32cw; break;
  }

  if (CRCIntrinsicID != Intrinsic::not_intrinsic) {
    Value *Arg0 = EmitScalarExpr(E->getArg(0));
    Value *Arg1 = EmitScalarExpr(E->getArg(1));

    // crc32{c,}d intrinsics are implemnted as two calls to crc32{c,}w
    // intrinsics, hence we need different codegen for these cases.
    if (BuiltinID == ARM::BI__builtin_arm_crc32d ||
        BuiltinID == ARM::BI__builtin_arm_crc32cd) {
      Value *C1 = llvm::ConstantInt::get(Int64Ty, 32);
      Value *Arg1a = Builder.CreateTruncOrBitCast(Arg1, Int32Ty);
      Value *Arg1b = Builder.CreateLShr(Arg1, C1);
      Arg1b = Builder.CreateTruncOrBitCast(Arg1b, Int32Ty);

      Function *F = CGM.getIntrinsic(CRCIntrinsicID);
      Value *Res = Builder.CreateCall2(F, Arg0, Arg1a);
      return Builder.CreateCall2(F, Res, Arg1b);
    } else {
      Arg1 = Builder.CreateZExtOrBitCast(Arg1, Int32Ty);

      Function *F = CGM.getIntrinsic(CRCIntrinsicID);
      return Builder.CreateCall2(F, Arg0, Arg1);
    }
  }

  SmallVector<Value*, 4> Ops;
  llvm::Value *Align = 0;
  for (unsigned i = 0, e = E->getNumArgs() - 1; i != e; i++) {
    if (i == 0) {
      switch (BuiltinID) {
      case NEON::BI__builtin_neon_vld1_v:
      case NEON::BI__builtin_neon_vld1q_v:
      case NEON::BI__builtin_neon_vld1q_lane_v:
      case NEON::BI__builtin_neon_vld1_lane_v:
      case NEON::BI__builtin_neon_vld1_dup_v:
      case NEON::BI__builtin_neon_vld1q_dup_v:
      case NEON::BI__builtin_neon_vst1_v:
      case NEON::BI__builtin_neon_vst1q_v:
      case NEON::BI__builtin_neon_vst1q_lane_v:
      case NEON::BI__builtin_neon_vst1_lane_v:
      case NEON::BI__builtin_neon_vst2_v:
      case NEON::BI__builtin_neon_vst2q_v:
      case NEON::BI__builtin_neon_vst2_lane_v:
      case NEON::BI__builtin_neon_vst2q_lane_v:
      case NEON::BI__builtin_neon_vst3_v:
      case NEON::BI__builtin_neon_vst3q_v:
      case NEON::BI__builtin_neon_vst3_lane_v:
      case NEON::BI__builtin_neon_vst3q_lane_v:
      case NEON::BI__builtin_neon_vst4_v:
      case NEON::BI__builtin_neon_vst4q_v:
      case NEON::BI__builtin_neon_vst4_lane_v:
      case NEON::BI__builtin_neon_vst4q_lane_v:
        // Get the alignment for the argument in addition to the value;
        // we'll use it later.
        std::pair<llvm::Value*, unsigned> Src =
            EmitPointerWithAlignment(E->getArg(0));
        Ops.push_back(Src.first);
        Align = Builder.getInt32(Src.second);
        continue;
      }
    }
    if (i == 1) {
      switch (BuiltinID) {
      case NEON::BI__builtin_neon_vld2_v:
      case NEON::BI__builtin_neon_vld2q_v:
      case NEON::BI__builtin_neon_vld3_v:
      case NEON::BI__builtin_neon_vld3q_v:
      case NEON::BI__builtin_neon_vld4_v:
      case NEON::BI__builtin_neon_vld4q_v:
      case NEON::BI__builtin_neon_vld2_lane_v:
      case NEON::BI__builtin_neon_vld2q_lane_v:
      case NEON::BI__builtin_neon_vld3_lane_v:
      case NEON::BI__builtin_neon_vld3q_lane_v:
      case NEON::BI__builtin_neon_vld4_lane_v:
      case NEON::BI__builtin_neon_vld4q_lane_v:
      case NEON::BI__builtin_neon_vld2_dup_v:
      case NEON::BI__builtin_neon_vld3_dup_v:
      case NEON::BI__builtin_neon_vld4_dup_v:
        // Get the alignment for the argument in addition to the value;
        // we'll use it later.
        std::pair<llvm::Value*, unsigned> Src =
            EmitPointerWithAlignment(E->getArg(1));
        Ops.push_back(Src.first);
        Align = Builder.getInt32(Src.second);
        continue;
      }
    }
    Ops.push_back(EmitScalarExpr(E->getArg(i)));
  }

  switch (BuiltinID) {
  default: break;
  // vget_lane and vset_lane are not overloaded and do not have an extra
  // argument that specifies the vector type.
  case NEON::BI__builtin_neon_vget_lane_i8:
  case NEON::BI__builtin_neon_vget_lane_i16:
  case NEON::BI__builtin_neon_vget_lane_i32:
  case NEON::BI__builtin_neon_vget_lane_i64:
  case NEON::BI__builtin_neon_vget_lane_f32:
  case NEON::BI__builtin_neon_vgetq_lane_i8:
  case NEON::BI__builtin_neon_vgetq_lane_i16:
  case NEON::BI__builtin_neon_vgetq_lane_i32:
  case NEON::BI__builtin_neon_vgetq_lane_i64:
  case NEON::BI__builtin_neon_vgetq_lane_f32:
    return Builder.CreateExtractElement(Ops[0], EmitScalarExpr(E->getArg(1)),
                                        "vget_lane");
  case NEON::BI__builtin_neon_vset_lane_i8:
  case NEON::BI__builtin_neon_vset_lane_i16:
  case NEON::BI__builtin_neon_vset_lane_i32:
  case NEON::BI__builtin_neon_vset_lane_i64:
  case NEON::BI__builtin_neon_vset_lane_f32:
  case NEON::BI__builtin_neon_vsetq_lane_i8:
  case NEON::BI__builtin_neon_vsetq_lane_i16:
  case NEON::BI__builtin_neon_vsetq_lane_i32:
  case NEON::BI__builtin_neon_vsetq_lane_i64:
  case NEON::BI__builtin_neon_vsetq_lane_f32:
    Ops.push_back(EmitScalarExpr(E->getArg(2)));
    return Builder.CreateInsertElement(Ops[1], Ops[0], Ops[2], "vset_lane");

  // Non-polymorphic crypto instructions also not overloaded
  case NEON::BI__builtin_neon_vsha1h_u32:
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_sha1h), Ops,
                        "vsha1h");
  case NEON::BI__builtin_neon_vsha1cq_u32:
    Ops.push_back(EmitScalarExpr(E->getArg(2)));
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_sha1c), Ops,
                        "vsha1h");
  case NEON::BI__builtin_neon_vsha1pq_u32:
    Ops.push_back(EmitScalarExpr(E->getArg(2)));
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_sha1p), Ops,
                        "vsha1h");
  case NEON::BI__builtin_neon_vsha1mq_u32:
    Ops.push_back(EmitScalarExpr(E->getArg(2)));
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_sha1m), Ops,
                        "vsha1h");
  }

  // Get the last argument, which specifies the vector type.
  llvm::APSInt Result;
  const Expr *Arg = E->getArg(E->getNumArgs()-1);
  if (!Arg->isIntegerConstantExpr(Result, getContext()))
    return 0;

  if (BuiltinID == ARM::BI__builtin_arm_vcvtr_f ||
      BuiltinID == ARM::BI__builtin_arm_vcvtr_d) {
    // Determine the overloaded type of this builtin.
    llvm::Type *Ty;
    if (BuiltinID == ARM::BI__builtin_arm_vcvtr_f)
      Ty = FloatTy;
    else
      Ty = DoubleTy;

    // Determine whether this is an unsigned conversion or not.
    bool usgn = Result.getZExtValue() == 1;
    unsigned Int = usgn ? Intrinsic::arm_vcvtru : Intrinsic::arm_vcvtr;

    // Call the appropriate intrinsic.
    Function *F = CGM.getIntrinsic(Int, Ty);
    return Builder.CreateCall(F, Ops, "vcvtr");
  }

  // Determine the type of this overloaded NEON intrinsic.
  NeonTypeFlags Type(Result.getZExtValue());
  bool usgn = Type.isUnsigned();
  bool rightShift = false;

  llvm::VectorType *VTy = GetNeonType(this, Type);
  llvm::Type *Ty = VTy;
  if (!Ty)
    return 0;


  // Many NEON builtins have identical semantics and uses in ARM and
  // AArch64. Emit these in a single function.
  unsigned LLVMIntrinsic = 0, AltLLVMIntrinsic = 0;
  findNeonIntrinsic(ARMNeonIntrinsicMap, BuiltinID, ARMIntrinsicsProvenSorted,
                    LLVMIntrinsic, AltLLVMIntrinsic);

  if (Value *Result = EmitCommonNeonBuiltinExpr(
          BuiltinID, LLVMIntrinsic, AltLLVMIntrinsic, E, Ops, Align))
    return Result;

  unsigned Int;
  switch (BuiltinID) {
  default: return 0;
  case NEON::BI__builtin_neon_vabs_v:
  case NEON::BI__builtin_neon_vabsq_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vabs, Ty),
                        Ops, "vabs");
  case NEON::BI__builtin_neon_vld1q_lane_v:
    // Handle 64-bit integer elements as a special case.  Use shuffles of
    // one-element vectors to avoid poor code for i64 in the backend.
    if (VTy->getElementType()->isIntegerTy(64)) {
      // Extract the other lane.
      Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
      int Lane = cast<ConstantInt>(Ops[2])->getZExtValue();
      Value *SV = llvm::ConstantVector::get(ConstantInt::get(Int32Ty, 1-Lane));
      Ops[1] = Builder.CreateShuffleVector(Ops[1], Ops[1], SV);
      // Load the value as a one-element vector.
      Ty = llvm::VectorType::get(VTy->getElementType(), 1);
      Function *F = CGM.getIntrinsic(Intrinsic::arm_neon_vld1, Ty);
      Value *Ld = Builder.CreateCall2(F, Ops[0], Align);
      // Combine them.
      SmallVector<Constant*, 2> Indices;
      Indices.push_back(ConstantInt::get(Int32Ty, 1-Lane));
      Indices.push_back(ConstantInt::get(Int32Ty, Lane));
      SV = llvm::ConstantVector::get(Indices);
      return Builder.CreateShuffleVector(Ops[1], Ld, SV, "vld1q_lane");
    }
    // fall through
  case NEON::BI__builtin_neon_vld1_lane_v: {
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ty = llvm::PointerType::getUnqual(VTy->getElementType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    LoadInst *Ld = Builder.CreateLoad(Ops[0]);
    Ld->setAlignment(cast<ConstantInt>(Align)->getZExtValue());
    return Builder.CreateInsertElement(Ops[1], Ld, Ops[2], "vld1_lane");
  }
  case NEON::BI__builtin_neon_vld2_dup_v:
  case NEON::BI__builtin_neon_vld3_dup_v:
  case NEON::BI__builtin_neon_vld4_dup_v: {
    // Handle 64-bit elements as a special-case.  There is no "dup" needed.
    if (VTy->getElementType()->getPrimitiveSizeInBits() == 64) {
      switch (BuiltinID) {
      case NEON::BI__builtin_neon_vld2_dup_v:
        Int = Intrinsic::arm_neon_vld2;
        break;
      case NEON::BI__builtin_neon_vld3_dup_v:
        Int = Intrinsic::arm_neon_vld3;
        break;
      case NEON::BI__builtin_neon_vld4_dup_v:
        Int = Intrinsic::arm_neon_vld4;
        break;
      default: llvm_unreachable("unknown vld_dup intrinsic?");
      }
      Function *F = CGM.getIntrinsic(Int, Ty);
      Ops[1] = Builder.CreateCall2(F, Ops[1], Align, "vld_dup");
      Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
      Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
      return Builder.CreateStore(Ops[1], Ops[0]);
    }
    switch (BuiltinID) {
    case NEON::BI__builtin_neon_vld2_dup_v:
      Int = Intrinsic::arm_neon_vld2lane;
      break;
    case NEON::BI__builtin_neon_vld3_dup_v:
      Int = Intrinsic::arm_neon_vld3lane;
      break;
    case NEON::BI__builtin_neon_vld4_dup_v:
      Int = Intrinsic::arm_neon_vld4lane;
      break;
    default: llvm_unreachable("unknown vld_dup intrinsic?");
    }
    Function *F = CGM.getIntrinsic(Int, Ty);
    llvm::StructType *STy = cast<llvm::StructType>(F->getReturnType());

    SmallVector<Value*, 6> Args;
    Args.push_back(Ops[1]);
    Args.append(STy->getNumElements(), UndefValue::get(Ty));

    llvm::Constant *CI = ConstantInt::get(Int32Ty, 0);
    Args.push_back(CI);
    Args.push_back(Align);

    Ops[1] = Builder.CreateCall(F, Args, "vld_dup");
    // splat lane 0 to all elts in each vector of the result.
    for (unsigned i = 0, e = STy->getNumElements(); i != e; ++i) {
      Value *Val = Builder.CreateExtractValue(Ops[1], i);
      Value *Elt = Builder.CreateBitCast(Val, Ty);
      Elt = EmitNeonSplat(Elt, CI);
      Elt = Builder.CreateBitCast(Elt, Val->getType());
      Ops[1] = Builder.CreateInsertValue(Ops[1], Elt, i);
    }
    Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vqrshrn_n_v:
    Int =
      usgn ? Intrinsic::arm_neon_vqrshiftnu : Intrinsic::arm_neon_vqrshiftns;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqrshrn_n",
                        1, true);
  case NEON::BI__builtin_neon_vqrshrun_n_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vqrshiftnsu, Ty),
                        Ops, "vqrshrun_n", 1, true);
  case NEON::BI__builtin_neon_vqshlu_n_v:
  case NEON::BI__builtin_neon_vqshluq_n_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vqshiftsu, Ty),
                        Ops, "vqshlu", 1, false);
  case NEON::BI__builtin_neon_vqshrn_n_v:
    Int = usgn ? Intrinsic::arm_neon_vqshiftnu : Intrinsic::arm_neon_vqshiftns;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqshrn_n",
                        1, true);
  case NEON::BI__builtin_neon_vqshrun_n_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vqshiftnsu, Ty),
                        Ops, "vqshrun_n", 1, true);
  case NEON::BI__builtin_neon_vrecpe_v:
  case NEON::BI__builtin_neon_vrecpeq_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vrecpe, Ty),
                        Ops, "vrecpe");
  case NEON::BI__builtin_neon_vrshrn_n_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vrshiftn, Ty),
                        Ops, "vrshrn_n", 1, true);
  case NEON::BI__builtin_neon_vrshr_n_v:
  case NEON::BI__builtin_neon_vrshrq_n_v:
    Int = usgn ? Intrinsic::arm_neon_vrshiftu : Intrinsic::arm_neon_vrshifts;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrshr_n", 1, true);
  case NEON::BI__builtin_neon_vrsra_n_v:
  case NEON::BI__builtin_neon_vrsraq_n_v:
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[2] = EmitNeonShiftVector(Ops[2], Ty, true);
    Int = usgn ? Intrinsic::arm_neon_vrshiftu : Intrinsic::arm_neon_vrshifts;
    Ops[1] = Builder.CreateCall2(CGM.getIntrinsic(Int, Ty), Ops[1], Ops[2]);
    return Builder.CreateAdd(Ops[0], Ops[1], "vrsra_n");
  case NEON::BI__builtin_neon_vshll_n_v:
    Int = usgn ? Intrinsic::arm_neon_vshiftlu : Intrinsic::arm_neon_vshiftls;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vshll", 1);
  case NEON::BI__builtin_neon_vsri_n_v:
  case NEON::BI__builtin_neon_vsriq_n_v:
    rightShift = true;
  case NEON::BI__builtin_neon_vsli_n_v:
  case NEON::BI__builtin_neon_vsliq_n_v:
    Ops[2] = EmitNeonShiftVector(Ops[2], Ty, rightShift);
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vshiftins, Ty),
                        Ops, "vsli_n");
  case NEON::BI__builtin_neon_vsra_n_v:
  case NEON::BI__builtin_neon_vsraq_n_v:
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = EmitNeonRShiftImm(Ops[1], Ops[2], Ty, usgn, "vsra_n");
    return Builder.CreateAdd(Ops[0], Ops[1]);
  case NEON::BI__builtin_neon_vst1q_lane_v:
    // Handle 64-bit integer elements as a special case.  Use a shuffle to get
    // a one-element vector and avoid poor code for i64 in the backend.
    if (VTy->getElementType()->isIntegerTy(64)) {
      Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
      Value *SV = llvm::ConstantVector::get(cast<llvm::Constant>(Ops[2]));
      Ops[1] = Builder.CreateShuffleVector(Ops[1], Ops[1], SV);
      Ops[2] = Align;
      return Builder.CreateCall(CGM.getIntrinsic(Intrinsic::arm_neon_vst1,
                                                 Ops[1]->getType()), Ops);
    }
    // fall through
  case NEON::BI__builtin_neon_vst1_lane_v: {
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[1] = Builder.CreateExtractElement(Ops[1], Ops[2]);
    Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
    StoreInst *St = Builder.CreateStore(Ops[1],
                                        Builder.CreateBitCast(Ops[0], Ty));
    St->setAlignment(cast<ConstantInt>(Align)->getZExtValue());
    return St;
  }
  case NEON::BI__builtin_neon_vtbl1_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vtbl1),
                        Ops, "vtbl1");
  case NEON::BI__builtin_neon_vtbl2_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vtbl2),
                        Ops, "vtbl2");
  case NEON::BI__builtin_neon_vtbl3_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vtbl3),
                        Ops, "vtbl3");
  case NEON::BI__builtin_neon_vtbl4_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vtbl4),
                        Ops, "vtbl4");
  case NEON::BI__builtin_neon_vtbx1_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vtbx1),
                        Ops, "vtbx1");
  case NEON::BI__builtin_neon_vtbx2_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vtbx2),
                        Ops, "vtbx2");
  case NEON::BI__builtin_neon_vtbx3_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vtbx3),
                        Ops, "vtbx3");
  case NEON::BI__builtin_neon_vtbx4_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm_neon_vtbx4),
                        Ops, "vtbx4");
  }
}

static unsigned getARM64IntrinsicFCVT(unsigned Builtin) {
  switch (Builtin) {
  default: assert(0 && "Unexpected builtin!"); break;
  case NEON::BI__builtin_neon_vcvtas_u32_f32:
    return Intrinsic::arm64_neon_fcvtas;
  case NEON::BI__builtin_neon_vcvtad_u64_f64:
    return Intrinsic::arm64_neon_fcvtau;
  case NEON::BI__builtin_neon_vcvtns_u32_f32:
    return Intrinsic::arm64_neon_fcvtns;
  case NEON::BI__builtin_neon_vcvtnd_u64_f64:
    return Intrinsic::arm64_neon_fcvtns;
  case NEON::BI__builtin_neon_vcvtms_u32_f32:
    return Intrinsic::arm64_neon_fcvtms;
  case NEON::BI__builtin_neon_vcvtmd_u64_f64:
    return Intrinsic::arm64_neon_fcvtms;
  case NEON::BI__builtin_neon_vcvtps_u32_f32:
    return Intrinsic::arm64_neon_fcvtps;
  case NEON::BI__builtin_neon_vcvtpd_u64_f64:
    return Intrinsic::arm64_neon_fcvtas;
  case NEON::BI__builtin_neon_vcvtas_s32_f32:
    return Intrinsic::arm64_neon_fcvtas;
  case NEON::BI__builtin_neon_vcvtad_s64_f64:
    return Intrinsic::arm64_neon_fcvtns;
  case NEON::BI__builtin_neon_vcvtns_s32_f32:
    return Intrinsic::arm64_neon_fcvtns;
  case NEON::BI__builtin_neon_vcvtnd_s64_f64:
    return Intrinsic::arm64_neon_fcvtms;
  case NEON::BI__builtin_neon_vcvtms_s32_f32:
    return Intrinsic::arm64_neon_fcvtms;
  case NEON::BI__builtin_neon_vcvtmd_s64_f64:
    return Intrinsic::arm64_neon_fcvtms;
  case NEON::BI__builtin_neon_vcvtps_s32_f32:
    return Intrinsic::arm64_neon_fcvtps;
  case NEON::BI__builtin_neon_vcvtpd_s64_f64:
    return Intrinsic::arm64_neon_fcvtps;
  }
  return 0;
}

Value *CodeGenFunction::vectorWrapScalar16(Value *Op) {
  llvm::Type *VTy = llvm::VectorType::get(Int16Ty, 4);
  Op = Builder.CreateBitCast(Op, Int16Ty);
  Value *V = UndefValue::get(VTy);
  llvm::Constant *CI = ConstantInt::get(Int32Ty, 0);
  Op = Builder.CreateInsertElement(V, Op, CI);
  return Op;
}

Value *CodeGenFunction::vectorWrapScalar8(Value *Op) {
  llvm::Type *VTy = llvm::VectorType::get(Int8Ty, 8);
  Op = Builder.CreateBitCast(Op, Int8Ty);
  Value *V = UndefValue::get(VTy);
  llvm::Constant *CI = ConstantInt::get(Int32Ty, 0);
  Op = Builder.CreateInsertElement(V, Op, CI);
  return Op;
}

Value *CodeGenFunction::
emitVectorWrappedScalar8Intrinsic(unsigned Int, SmallVectorImpl<Value*> &Ops,
                                  const char *Name) {
  // i8 is not a legal types for ARM64, so we can't just use
  // a normal overloaed intrinsic call for these scalar types. Instead
  // we'll build 64-bit vectors w/ lane zero being our input values and
  // perform the operation on that. The back end can pattern match directly
  // to the scalar instruction.
  Ops[0] = vectorWrapScalar8(Ops[0]);
  Ops[1] = vectorWrapScalar8(Ops[1]);
  llvm::Type *VTy = llvm::VectorType::get(Int8Ty, 8);
  Value *V = EmitNeonCall(CGM.getIntrinsic(Int, VTy), Ops, Name);
  Constant *CI = ConstantInt::get(Int32Ty, 0);
  return Builder.CreateExtractElement(V, CI, "lane0");
}

Value *CodeGenFunction::
emitVectorWrappedScalar16Intrinsic(unsigned Int, SmallVectorImpl<Value*> &Ops,
                                   const char *Name) {
  // i16 is not a legal types for ARM64, so we can't just use
  // a normal overloaed intrinsic call for these scalar types. Instead
  // we'll build 64-bit vectors w/ lane zero being our input values and
  // perform the operation on that. The back end can pattern match directly
  // to the scalar instruction.
  Ops[0] = vectorWrapScalar16(Ops[0]);
  Ops[1] = vectorWrapScalar16(Ops[1]);
  llvm::Type *VTy = llvm::VectorType::get(Int16Ty, 4);
  Value *V = EmitNeonCall(CGM.getIntrinsic(Int, VTy), Ops, Name);
  Constant *CI = ConstantInt::get(Int32Ty, 0);
  return Builder.CreateExtractElement(V, CI, "lane0");
}

Value *CodeGenFunction::EmitARM64BuiltinExpr(unsigned BuiltinID,
                                             const CallExpr *E) {
  if (BuiltinID == ARM64::BI__clear_cache) {
    assert(E->getNumArgs() == 2 && "__clear_cache takes 2 arguments");
    const FunctionDecl *FD = E->getDirectCallee();
    SmallVector<Value*, 2> Ops;
    for (unsigned i = 0; i < 2; i++)
      Ops.push_back(EmitScalarExpr(E->getArg(i)));
    llvm::Type *Ty = CGM.getTypes().ConvertType(FD->getType());
    llvm::FunctionType *FTy = cast<llvm::FunctionType>(Ty);
    StringRef Name = FD->getName();
    return EmitNounwindRuntimeCall(CGM.CreateRuntimeFunction(FTy, Name), Ops);
  }

  if (BuiltinID == ARM64::BI__builtin_arm_ldrex &&
      getContext().getTypeSize(E->getType()) == 128) {
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_ldxp);

    Value *LdPtr = EmitScalarExpr(E->getArg(0));
    Value *Val = Builder.CreateCall(F, Builder.CreateBitCast(LdPtr, Int8PtrTy),
                                    "ldxp");

    Value *Val0 = Builder.CreateExtractValue(Val, 1);
    Value *Val1 = Builder.CreateExtractValue(Val, 0);
    llvm::Type *Int128Ty = llvm::IntegerType::get(getLLVMContext(), 128);
    Val0 = Builder.CreateZExt(Val0, Int128Ty);
    Val1 = Builder.CreateZExt(Val1, Int128Ty);

    Value *ShiftCst = llvm::ConstantInt::get(Int128Ty, 64);
    Val = Builder.CreateShl(Val0, ShiftCst, "shl", true /* nuw */);
    Val = Builder.CreateOr(Val, Val1);
    return Builder.CreateBitCast(Val, ConvertType(E->getType()));
  } else if (BuiltinID == ARM64::BI__builtin_arm_ldrex) {
    Value *LoadAddr = EmitScalarExpr(E->getArg(0));

    QualType Ty = E->getType();
    llvm::Type *RealResTy = ConvertType(Ty);
    llvm::Type *IntResTy = llvm::IntegerType::get(getLLVMContext(),
                                                  getContext().getTypeSize(Ty));
    LoadAddr = Builder.CreateBitCast(LoadAddr, IntResTy->getPointerTo());

    Function *F = CGM.getIntrinsic(Intrinsic::arm64_ldxr, LoadAddr->getType());
    Value *Val = Builder.CreateCall(F, LoadAddr, "ldxr");

    if (RealResTy->isPointerTy())
      return Builder.CreateIntToPtr(Val, RealResTy);

    Val = Builder.CreateTruncOrBitCast(Val, IntResTy);
    return Builder.CreateBitCast(Val, RealResTy);
  }

  if (BuiltinID == ARM64::BI__builtin_arm_strex &&
      getContext().getTypeSize(E->getArg(0)->getType()) == 128) {
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_stxp);
    llvm::Type *STy = llvm::StructType::get(Int64Ty, Int64Ty, NULL);

    Value *One = llvm::ConstantInt::get(Int32Ty, 1);
    Value *Tmp = Builder.CreateAlloca(ConvertType(E->getArg(0)->getType()),
                                      One);
    Value *Val = EmitScalarExpr(E->getArg(0));
    Builder.CreateStore(Val, Tmp);

    Value *LdPtr = Builder.CreateBitCast(Tmp,llvm::PointerType::getUnqual(STy));
    Val = Builder.CreateLoad(LdPtr);

    Value *Arg0 = Builder.CreateExtractValue(Val, 0);
    Value *Arg1 = Builder.CreateExtractValue(Val, 1);
    Value *StPtr = Builder.CreateBitCast(EmitScalarExpr(E->getArg(1)),
                                         Int8PtrTy);
    return Builder.CreateCall3(F, Arg0, Arg1, StPtr, "stxp");
  } else if (BuiltinID == ARM64::BI__builtin_arm_strex) {
    Value *StoreVal = EmitScalarExpr(E->getArg(0));
    Value *StoreAddr = EmitScalarExpr(E->getArg(1));

    QualType Ty = E->getArg(0)->getType();
    llvm::Type *StoreTy = llvm::IntegerType::get(getLLVMContext(),
                                                 getContext().getTypeSize(Ty));
    StoreAddr = Builder.CreateBitCast(StoreAddr, StoreTy->getPointerTo());

    if (StoreVal->getType()->isPointerTy())
      StoreVal = Builder.CreatePtrToInt(StoreVal, Int64Ty);
    else {
      StoreVal = Builder.CreateBitCast(StoreVal, StoreTy);
      StoreVal = Builder.CreateZExtOrBitCast(StoreVal, Int64Ty);
    }

    Function *F = CGM.getIntrinsic(Intrinsic::arm64_stxr, StoreAddr->getType());
    return Builder.CreateCall2(F, StoreVal, StoreAddr, "stxr");
  }

  if (BuiltinID == ARM64::BI__builtin_arm_clrex) {
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_clrex);
    return Builder.CreateCall(F);
  }

  // CRC32
  Intrinsic::ID CRCIntrinsicID = Intrinsic::not_intrinsic;
  switch (BuiltinID) {
  case ARM64::BI__builtin_arm_crc32b:
    CRCIntrinsicID = Intrinsic::arm64_crc32b; break;
  case ARM64::BI__builtin_arm_crc32cb:
    CRCIntrinsicID = Intrinsic::arm64_crc32cb; break;
  case ARM64::BI__builtin_arm_crc32h:
    CRCIntrinsicID = Intrinsic::arm64_crc32h; break;
  case ARM64::BI__builtin_arm_crc32ch:
    CRCIntrinsicID = Intrinsic::arm64_crc32ch; break;
  case ARM64::BI__builtin_arm_crc32w:
    CRCIntrinsicID = Intrinsic::arm64_crc32w; break;
  case ARM64::BI__builtin_arm_crc32cw:
    CRCIntrinsicID = Intrinsic::arm64_crc32cw; break;
  case ARM64::BI__builtin_arm_crc32d:
    CRCIntrinsicID = Intrinsic::arm64_crc32x; break;
  case ARM64::BI__builtin_arm_crc32cd:
    CRCIntrinsicID = Intrinsic::arm64_crc32cx; break;
  }

  if (CRCIntrinsicID != Intrinsic::not_intrinsic) {
    Value *Arg0 = EmitScalarExpr(E->getArg(0));
    Value *Arg1 = EmitScalarExpr(E->getArg(1));
    Function *F = CGM.getIntrinsic(CRCIntrinsicID);

    llvm::Type *DataTy = F->getFunctionType()->getParamType(1);
    Arg1 = Builder.CreateZExtOrBitCast(Arg1, DataTy);

    return Builder.CreateCall2(F, Arg0, Arg1);
  }

  llvm::SmallVector<Value*, 4> Ops;
  for (unsigned i = 0, e = E->getNumArgs() - 1; i != e; i++)
    Ops.push_back(EmitScalarExpr(E->getArg(i)));

  llvm::APSInt Result;
  const Expr *Arg = E->getArg(E->getNumArgs()-1);
  NeonTypeFlags Type(0);
  if (Arg->isIntegerConstantExpr(Result, getContext()))
    // Determine the type of this overloaded NEON intrinsic.
    Type = NeonTypeFlags(Result.getZExtValue());

  bool usgn = Type.isUnsigned();
  bool quad = Type.isQuad();

  // Handle non-overloaded intrinsics first.
  switch (BuiltinID) {
  default: break;
  case NEON::BI__builtin_neon_vrsqrtss_f32:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_frsqrts,
                                         FloatTy),
                        Ops, "vrsqrtss");

  case NEON::BI__builtin_neon_vrsqrtsd_f64:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_frsqrts,
                                         DoubleTy),
                        Ops, "vrsqrtsd");

  case NEON::BI__builtin_neon_vrshl_u64:
  case NEON::BI__builtin_neon_vrshld_u64:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vrshl_s64:
  case NEON::BI__builtin_neon_vrshld_s64: {
    unsigned Int = usgn ? Intrinsic::arm64_neon_urshl :
      Intrinsic::arm64_neon_srshl;
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Int64Ty), Ops, "vrshld");
  }
  case NEON::BI__builtin_neon_vqrshlb_u8:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar8Intrinsic(Intrinsic::arm64_neon_uqrshl,
                                             Ops, "vqrshlb");
  case NEON::BI__builtin_neon_vqrshlb_s8:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar8Intrinsic(Intrinsic::arm64_neon_sqrshl,
                                             Ops, "vqrshlb");
  case NEON::BI__builtin_neon_vqrshlh_u16:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar16Intrinsic(Intrinsic::arm64_neon_uqrshl,
                                              Ops, "vqrshlh");
  case NEON::BI__builtin_neon_vqrshlh_s16:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar16Intrinsic(Intrinsic::arm64_neon_sqrshl,
                                              Ops, "vqrshlh");
  case NEON::BI__builtin_neon_vqrshls_u32:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vqrshls_s32: {
    unsigned Int = usgn ? Intrinsic::arm64_neon_uqrshl :
      Intrinsic::arm64_neon_sqrshl;
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Int32Ty), Ops, "vqrshls");
  }
  case NEON::BI__builtin_neon_vqrshl_u64:
  case NEON::BI__builtin_neon_vqrshld_u64:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vqrshl_s64:
  case NEON::BI__builtin_neon_vqrshld_s64: {
    unsigned Int = usgn ? Intrinsic::arm64_neon_uqrshl :
      Intrinsic::arm64_neon_sqrshl;
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Int64Ty), Ops, "vqrshld");
  }

  case NEON::BI__builtin_neon_vqshlb_u8:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar8Intrinsic(Intrinsic::arm64_neon_uqshl,
                                             Ops, "vqshlb");
  case NEON::BI__builtin_neon_vqshlb_s8:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar8Intrinsic(Intrinsic::arm64_neon_sqshl,
                                             Ops, "vqshlb");
  case NEON::BI__builtin_neon_vqshlh_u16:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar16Intrinsic(Intrinsic::arm64_neon_uqshl,
                                              Ops, "vqshlh");
  case NEON::BI__builtin_neon_vqshlh_s16:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar16Intrinsic(Intrinsic::arm64_neon_sqshl,
                                              Ops, "vqshlh");
  case NEON::BI__builtin_neon_vqshls_u32:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vqshls_s32: {
    unsigned Int = usgn ? Intrinsic::arm64_neon_uqshl :
      Intrinsic::arm64_neon_sqshl;
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Int32Ty), Ops, "vqshls");
  }
  case NEON::BI__builtin_neon_vqshl_u64:
  case NEON::BI__builtin_neon_vqshld_u64:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vqshl_s64:
  case NEON::BI__builtin_neon_vqshld_s64: {
    unsigned Int = usgn ? Intrinsic::arm64_neon_uqshl :
      Intrinsic::arm64_neon_sqshl;
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Int64Ty), Ops, "vqshld");
  }
  case NEON::BI__builtin_neon_vshl_u64:
  case NEON::BI__builtin_neon_vshld_u64:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vshl_s64:
  case NEON::BI__builtin_neon_vshld_s64: {
    unsigned Int = usgn ? Intrinsic::arm64_neon_ushl :
      Intrinsic::arm64_neon_sshl;
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Int64Ty), Ops, "vshl");
  }
  case NEON::BI__builtin_neon_vqdmullh_lane_s16: {
    unsigned Int = Intrinsic::arm64_neon_sqdmull;
    // i16 is not a legal types for ARM64, so we can't just use
    // a normal overloaed intrinsic call for these scalar types. Instead
    // we'll build 64-bit vectors w/ lane zero being our input values and
    // perform the operation on that. The back end can pattern match directly
    // to the scalar instruction.
    Value *Idx = EmitScalarExpr(E->getArg(2));
    llvm::Type *WideVTy = llvm::VectorType::get(Int32Ty, 4);
    Ops[0] = vectorWrapScalar16(Ops[0]);
    Ops[1] = Builder.CreateExtractElement(Ops[1], Idx);
    Ops[1] = vectorWrapScalar16(Ops[1]);
    Value *V = EmitNeonCall(CGM.getIntrinsic(Int, WideVTy), Ops, "vqdmullh");
    Constant *CI = ConstantInt::get(Int32Ty, 0);
    return Builder.CreateExtractElement(V, CI, "lane0");
  }
  case NEON::BI__builtin_neon_vqdmulls_lane_s32: {
    unsigned Int = Intrinsic::arm64_neon_sqdmulls_scalar;
    Value *Idx = EmitScalarExpr(E->getArg(2));
    Ops[1] = Builder.CreateExtractElement(Ops[1], Idx, "lane");
    return EmitNeonCall(CGM.getIntrinsic(Int), Ops, "vqdmulls");
  }
  case NEON::BI__builtin_neon_vqdmulls_s32: {
    unsigned Int = Intrinsic::arm64_neon_sqdmulls_scalar;
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return EmitNeonCall(CGM.getIntrinsic(Int), Ops, "vqdmulls");
  }
  case NEON::BI__builtin_neon_vqdmullh_s16: {
    unsigned Int = Intrinsic::arm64_neon_sqdmull;
    // i16 is not a legal types for ARM64, so we can't just use
    // a normal overloaed intrinsic call for these scalar types. Instead
    // we'll build 64-bit vectors w/ lane zero being our input values and
    // perform the operation on that. The back end can pattern match directly
    // to the scalar instruction.
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    Ops[0] = vectorWrapScalar16(Ops[0]);
    Ops[1] = vectorWrapScalar16(Ops[1]);
    llvm::Type *WideVTy = llvm::VectorType::get(Int32Ty, 4);
    llvm::Constant *CI = ConstantInt::get(Int32Ty, 0);
    Value *V = EmitNeonCall(CGM.getIntrinsic(Int, WideVTy), Ops, "vqdmullh");
    return Builder.CreateExtractElement(V, CI, "lane0");
  }
  case NEON::BI__builtin_neon_vqaddb_u8:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar8Intrinsic(Intrinsic::arm64_neon_uqadd,
                                             Ops, "vqaddb");
  case NEON::BI__builtin_neon_vqaddb_s8:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar8Intrinsic(Intrinsic::arm64_neon_sqadd,
                                             Ops, "vqaddb");
  case NEON::BI__builtin_neon_vqaddh_u16:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar16Intrinsic(Intrinsic::arm64_neon_uqadd,
                                              Ops, "vqaddh");
  case NEON::BI__builtin_neon_vqaddh_s16:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar16Intrinsic(Intrinsic::arm64_neon_sqadd,
                                              Ops, "vqaddh");
  case NEON::BI__builtin_neon_vqadds_u32:
  case NEON::BI__builtin_neon_vqaddd_u64:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vqadds_s32:
  case NEON::BI__builtin_neon_vqaddd_s64: {
    unsigned Int = usgn ? Intrinsic::arm64_neon_uqadd :
                          Intrinsic::arm64_neon_sqadd;
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    llvm::Type *Ty = Ops[0]->getType();
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqadd");
  }
  case NEON::BI__builtin_neon_vqsubb_u8:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar8Intrinsic(Intrinsic::arm64_neon_uqsub,
                                             Ops, "vqsubb");
  case NEON::BI__builtin_neon_vqsubb_s8:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar8Intrinsic(Intrinsic::arm64_neon_sqsub,
                                             Ops, "vqsubb");
  case NEON::BI__builtin_neon_vqsubh_u16:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar16Intrinsic(Intrinsic::arm64_neon_uqsub,
                                              Ops, "vqsubh");
  case NEON::BI__builtin_neon_vqsubh_s16:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return emitVectorWrappedScalar16Intrinsic(Intrinsic::arm64_neon_sqsub,
                                              Ops, "vqsubh");

  case NEON::BI__builtin_neon_vqsubs_u32:
  case NEON::BI__builtin_neon_vqsubd_u64:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vqsubs_s32:
  case NEON::BI__builtin_neon_vqsubd_s64: {
    unsigned Int = usgn ? Intrinsic::arm64_neon_uqsub :
                          Intrinsic::arm64_neon_sqsub;
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    llvm::Type *Ty = Ops[0]->getType();
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqsub");
  }
  case NEON::BI__builtin_neon_vqmovnh_u16:
  case NEON::BI__builtin_neon_vqmovns_u32:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vqmovnh_s16:
  case NEON::BI__builtin_neon_vqmovns_s32: {
    unsigned Int = usgn ? Intrinsic::arm64_neon_uqxtn :
                          Intrinsic::arm64_neon_sqxtn;
    // i8 and i16 are not legal types for ARM64, so we can't just use
    // a normal overloaed intrinsic call for these scalar types. Instead
    // we'll build 64-bit vectors w/ lane zero being our input values and
    // perform the operation on that. The back end can pattern match directly
    // to the scalar instruction.
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    unsigned BitWidth =
      (BuiltinID == NEON::BI__builtin_neon_vqmovnh_s16 ||
       BuiltinID == NEON::BI__builtin_neon_vqmovnh_u16) ? 8 : 16;
    unsigned NumElts = 64 / BitWidth;
    llvm::Type *ResEltTy = BitWidth == 8 ? Int8Ty : Int16Ty;
    llvm::Type *ArgEltTy = BitWidth == 8 ? Int16Ty : Int32Ty;
    llvm::Type *ResTy = llvm::VectorType::get(ResEltTy, NumElts);
    llvm::Type *ArgTy = llvm::VectorType::get(ArgEltTy, NumElts);
    Ops[0] = Builder.CreateBitCast(Ops[0], ArgEltTy);
    Value *V = UndefValue::get(ArgTy);
    llvm::Constant *CI = ConstantInt::get(Int32Ty, 0);
    Ops[0] = Builder.CreateInsertElement(V, Ops[0], CI);
    V = EmitNeonCall(CGM.getIntrinsic(Int, ResTy), Ops, "vqmovn");
    return Builder.CreateExtractElement(V, CI, "lane0");
  }
  case NEON::BI__builtin_neon_vqmovnd_u64:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vqmovnd_s64: {
    unsigned Int = usgn ? Intrinsic::arm64_neon_scalar_uqxtn :
                          Intrinsic::arm64_neon_scalar_sqxtn;
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    llvm::Type *Tys[2] = { Int32Ty, Int64Ty };
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vqmovn");
  }
  case NEON::BI__builtin_neon_vqmovunh_s16:
  case NEON::BI__builtin_neon_vqmovuns_s32: {
    unsigned Int = Intrinsic::arm64_neon_sqxtun;
    // i8 and i16 are not legal types for ARM64, so we can't just use
    // a normal overloaed intrinsic call for these scalar types. Instead
    // we'll build 64-bit vectors w/ lane zero being our input values and
    // perform the operation on that. The back end can pattern match directly
    // to the scalar instruction.
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    unsigned BitWidth =
      BuiltinID == NEON::BI__builtin_neon_vqmovunh_s16 ? 8 : 16;
    unsigned NumElts = 64 / BitWidth;
    llvm::Type *ResEltTy = BitWidth == 8 ? Int8Ty : Int16Ty;
    llvm::Type *ArgEltTy = BitWidth == 8 ? Int16Ty : Int32Ty;
    llvm::Type *ResTy = llvm::VectorType::get(ResEltTy, NumElts);
    llvm::Type *ArgTy = llvm::VectorType::get(ArgEltTy, NumElts);
    Ops[0] = Builder.CreateBitCast(Ops[0], ArgEltTy);
    Value *V = UndefValue::get(ArgTy);
    llvm::Constant *CI = ConstantInt::get(Int32Ty, 0);
    Ops[0] = Builder.CreateInsertElement(V, Ops[0], CI);
    V = EmitNeonCall(CGM.getIntrinsic(Int, ResTy), Ops, "vqmovun");
    return Builder.CreateExtractElement(V, CI, "lane0");
  }
  case NEON::BI__builtin_neon_vqmovund_s64: {
    unsigned Int = Intrinsic::arm64_neon_scalar_sqxtun;
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    llvm::Type *Tys[2] = { Int32Ty, Int64Ty };
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vqmovun");
  }
  case NEON::BI__builtin_neon_vqabsh_s16:
  case NEON::BI__builtin_neon_vqabsb_s8: {
    unsigned Int = Intrinsic::arm64_neon_sqabs;
    // i8 and i16 are not legal types for ARM64, so we can't just use
    // a normal overloaed intrinsic call for these scalar types. Instead
    // we'll build 64-bit vectors w/ lane zero being our input values and
    // perform the operation on that. The back end can pattern match directly
    // to the scalar instruction.
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    unsigned BitWidth = BuiltinID == NEON::BI__builtin_neon_vqabsb_s8 ? 8:16;
    unsigned NumElts = 64 / BitWidth;
    llvm::Type *EltTy = BitWidth == 8 ? Int8Ty : Int16Ty;
    llvm::Type *VTy = llvm::VectorType::get(EltTy, NumElts);
    Ops[0] = Builder.CreateBitCast(Ops[0], EltTy);
    Value *V = UndefValue::get(VTy);
    llvm::Constant *CI = ConstantInt::get(Int32Ty, 0);
    Ops[0] = Builder.CreateInsertElement(V, Ops[0], CI);
    V = EmitNeonCall(CGM.getIntrinsic(Int, VTy), Ops, "vqabs");
    return Builder.CreateExtractElement(V, CI, "lane0");
  }
  case NEON::BI__builtin_neon_vqabss_s32:
  case NEON::BI__builtin_neon_vqabsd_s64: {
    unsigned Int = Intrinsic::arm64_neon_sqabs;
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    llvm::Type *Ty = Ops[0]->getType();
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqabs");
  }
  case NEON::BI__builtin_neon_vqnegh_s16:
  case NEON::BI__builtin_neon_vqnegb_s8: {
    unsigned Int = Intrinsic::arm64_neon_sqneg;
    // i8 and i16 are not legal types for ARM64, so we can't just use
    // a normal overloaed intrinsic call for these scalar types. Instead
    // we'll build 64-bit vectors w/ lane zero being our input values and
    // perform the operation on that. The back end can pattern match directly
    // to the scalar instruction.
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    unsigned BitWidth = BuiltinID == NEON::BI__builtin_neon_vqnegb_s8 ? 8:16;
    unsigned NumElts = 64 / BitWidth;
    llvm::Type *EltTy = BitWidth == 8 ? Int8Ty : Int16Ty;
    llvm::Type *VTy = llvm::VectorType::get(EltTy, NumElts);
    Ops[0] = Builder.CreateBitCast(Ops[0], EltTy);
    Value *V = UndefValue::get(VTy);
    llvm::Constant *CI = ConstantInt::get(Int32Ty, 0);
    Ops[0] = Builder.CreateInsertElement(V, Ops[0], CI);
    V = EmitNeonCall(CGM.getIntrinsic(Int, VTy), Ops, "vqneg");
    return Builder.CreateExtractElement(V, CI, "lane0");
  }
  case NEON::BI__builtin_neon_vqnegs_s32:
  case NEON::BI__builtin_neon_vqnegd_s64: {
    unsigned Int = Intrinsic::arm64_neon_sqneg;
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    llvm::Type *Ty = Ops[0]->getType();
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqneg");
  }
  case NEON::BI__builtin_neon_vcvtd_n_f64_u64:
  case NEON::BI__builtin_neon_vcvts_n_f32_u32:
  case NEON::BI__builtin_neon_vcvtd_n_f64_s64:
  case NEON::BI__builtin_neon_vcvts_n_f32_s32: {
    usgn = (BuiltinID == NEON::BI__builtin_neon_vcvtd_n_f64_u64 ||
            BuiltinID == NEON::BI__builtin_neon_vcvts_n_f32_u32);
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    bool Is64 = Ops[0]->getType()->getPrimitiveSizeInBits() == 64;
    llvm::Type *InTy = Is64 ? Int64Ty : Int32Ty;
    llvm::Type *FTy = Is64 ? DoubleTy : FloatTy;
    llvm::Type *Tys[2] = { FTy, InTy };
    unsigned Int = usgn ? Intrinsic::arm64_neon_vcvtfxu2fp
                        : Intrinsic::arm64_neon_vcvtfxs2fp;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "fcvt_n");
  }
  case NEON::BI__builtin_neon_vcvts_n_u32_f32:
  case NEON::BI__builtin_neon_vcvtd_n_u64_f64:
  case NEON::BI__builtin_neon_vcvts_n_s32_f32:
  case NEON::BI__builtin_neon_vcvtd_n_s64_f64: {
    usgn = (BuiltinID == NEON::BI__builtin_neon_vcvts_n_u32_f32 ||
            BuiltinID == NEON::BI__builtin_neon_vcvtd_n_u64_f64);
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    bool Is64 = Ops[0]->getType()->getPrimitiveSizeInBits() == 64;
    llvm::Type *InTy = Is64 ? Int64Ty : Int32Ty;
    llvm::Type *FTy = Is64 ? DoubleTy : FloatTy;
    llvm::Type *Tys[2] = { InTy, FTy };
    unsigned Int = usgn ? Intrinsic::arm64_neon_vcvtfp2fxu
                        : Intrinsic::arm64_neon_vcvtfp2fxs;
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "fcvt_n");
  }
  case NEON::BI__builtin_neon_vcvtas_u32_f32:
  case NEON::BI__builtin_neon_vcvtad_u64_f64:
  case NEON::BI__builtin_neon_vcvtns_u32_f32:
  case NEON::BI__builtin_neon_vcvtnd_u64_f64:
  case NEON::BI__builtin_neon_vcvtms_u32_f32:
  case NEON::BI__builtin_neon_vcvtmd_u64_f64:
  case NEON::BI__builtin_neon_vcvtps_u32_f32:
  case NEON::BI__builtin_neon_vcvtpd_u64_f64:
  case NEON::BI__builtin_neon_vcvtas_s32_f32:
  case NEON::BI__builtin_neon_vcvtad_s64_f64:
  case NEON::BI__builtin_neon_vcvtns_s32_f32:
  case NEON::BI__builtin_neon_vcvtnd_s64_f64:
  case NEON::BI__builtin_neon_vcvtms_s32_f32:
  case NEON::BI__builtin_neon_vcvtmd_s64_f64:
  case NEON::BI__builtin_neon_vcvtps_s32_f32:
  case NEON::BI__builtin_neon_vcvtpd_s64_f64: {
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    bool Is64 = Ops[0]->getType()->getPrimitiveSizeInBits() == 64;
    llvm::Type *InTy = Is64 ? Int64Ty : Int32Ty;
    llvm::Type *FTy = Is64 ? DoubleTy : FloatTy;
    Ops[0] = Builder.CreateBitCast(Ops[0], FTy);
    llvm::Type *Tys[2] = { InTy, FTy };
    unsigned Int = getARM64IntrinsicFCVT(BuiltinID);
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "fcvtr");
  }

  case NEON::BI__builtin_neon_vcvtzs_u32_f32:
  case NEON::BI__builtin_neon_vcvtzd_u64_f64:
  case NEON::BI__builtin_neon_vcvts_u32_f32:
  case NEON::BI__builtin_neon_vcvtd_u64_f64:
    usgn = true;
    // FALL THROUGH
  case NEON::BI__builtin_neon_vcvtzs_s32_f32:
  case NEON::BI__builtin_neon_vcvtzd_s64_f64:
  case NEON::BI__builtin_neon_vcvts_s32_f32:
  case NEON::BI__builtin_neon_vcvtd_s64_f64: {
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    bool Is64 = Ops[0]->getType()->getPrimitiveSizeInBits() == 64;
    llvm::Type *InTy = Is64 ? Int64Ty : Int32Ty;
    llvm::Type *FTy = Is64 ? DoubleTy : FloatTy;
    Ops[0] = Builder.CreateBitCast(Ops[0], FTy);
    if (usgn)
      return Builder.CreateFPToUI(Ops[0], InTy);
    return Builder.CreateFPToSI(Ops[0], InTy);
  }
  case NEON::BI__builtin_neon_vcvts_f32_u32:
  case NEON::BI__builtin_neon_vcvtd_f64_u64:
    usgn = true;
    // FALL THROUGH
  case NEON::BI__builtin_neon_vcvts_f32_s32:
  case NEON::BI__builtin_neon_vcvtd_f64_s64: {
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    bool Is64 = Ops[0]->getType()->getPrimitiveSizeInBits() == 64;
    llvm::Type *InTy = Is64 ? Int64Ty : Int32Ty;
    llvm::Type *FTy = Is64 ? DoubleTy : FloatTy;
    Ops[0] = Builder.CreateBitCast(Ops[0], InTy);
    if (usgn)
      return Builder.CreateUIToFP(Ops[0], FTy);
    return Builder.CreateSIToFP(Ops[0], FTy);
  }
  case NEON::BI__builtin_neon_vcvtxd_f32_f64: {
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_sisd_fcvtxn), Ops,
                        "vcvtxd");
  }
  case NEON::BI__builtin_neon_vpaddd_s64: {
    llvm::Type *Ty =
      llvm::VectorType::get(llvm::Type::getInt64Ty(getLLVMContext()), 2);
    Value *Vec = EmitScalarExpr(E->getArg(0));
    // The vector is v2f64, so make sure it's bitcast to that.
    Vec = Builder.CreateBitCast(Vec, Ty, "v2i64");
    llvm::Value *Idx0 = llvm::ConstantInt::get(Int32Ty, 0);
    llvm::Value *Idx1 = llvm::ConstantInt::get(Int32Ty, 1);
    Value *Op0 = Builder.CreateExtractElement(Vec, Idx0, "lane0");
    Value *Op1 = Builder.CreateExtractElement(Vec, Idx1, "lane1");
    // Pairwise addition of a v2f64 into a scalar f64.
    return Builder.CreateAdd(Op0, Op1, "vpaddd");
  }
  case NEON::BI__builtin_neon_vpaddd_f64: {
    llvm::Type *Ty =
      llvm::VectorType::get(llvm::Type::getDoubleTy(getLLVMContext()), 2);
    Value *Vec = EmitScalarExpr(E->getArg(0));
    // The vector is v2f64, so make sure it's bitcast to that.
    Vec = Builder.CreateBitCast(Vec, Ty, "v2f64");
    llvm::Value *Idx0 = llvm::ConstantInt::get(Int32Ty, 0);
    llvm::Value *Idx1 = llvm::ConstantInt::get(Int32Ty, 1);
    Value *Op0 = Builder.CreateExtractElement(Vec, Idx0, "lane0");
    Value *Op1 = Builder.CreateExtractElement(Vec, Idx1, "lane1");
    // Pairwise addition of a v2f64 into a scalar f64.
    return Builder.CreateFAdd(Op0, Op1, "vpaddd");
  }
  case NEON::BI__builtin_neon_vpadds_f32: {
    llvm::Type *Ty =
      llvm::VectorType::get(llvm::Type::getFloatTy(getLLVMContext()), 2);
    Value *Vec = EmitScalarExpr(E->getArg(0));
    // The vector is v2f32, so make sure it's bitcast to that.
    Vec = Builder.CreateBitCast(Vec, Ty, "v2f32");
    llvm::Value *Idx0 = llvm::ConstantInt::get(Int32Ty, 0);
    llvm::Value *Idx1 = llvm::ConstantInt::get(Int32Ty, 1);
    Value *Op0 = Builder.CreateExtractElement(Vec, Idx0, "lane0");
    Value *Op1 = Builder.CreateExtractElement(Vec, Idx1, "lane1");
    // Pairwise addition of a v2f32 into a scalar f32.
    return Builder.CreateFAdd(Op0, Op1, "vpaddd");
  }
  case NEON::BI__builtin_neon_vceqzd_s64:
  case NEON::BI__builtin_neon_vceqzd_u64:
  case NEON::BI__builtin_neon_vcgtzd_s64:
  case NEON::BI__builtin_neon_vcltzd_s64:
  case NEON::BI__builtin_neon_vcgezd_u64:
  case NEON::BI__builtin_neon_vcgezd_s64:
  case NEON::BI__builtin_neon_vclezd_u64:
  case NEON::BI__builtin_neon_vclezd_s64: {
    llvm::CmpInst::Predicate P;
    switch (BuiltinID) {
    default: llvm_unreachable("missing builtin ID in switch!");
    case NEON::BI__builtin_neon_vceqzd_s64:
    case NEON::BI__builtin_neon_vceqzd_u64:P = llvm::ICmpInst::ICMP_EQ;break;
    case NEON::BI__builtin_neon_vcgtzd_s64:P = llvm::ICmpInst::ICMP_SGT;break;
    case NEON::BI__builtin_neon_vcltzd_s64:P = llvm::ICmpInst::ICMP_SLT;break;
    case NEON::BI__builtin_neon_vcgezd_u64:P = llvm::ICmpInst::ICMP_UGE;break;
    case NEON::BI__builtin_neon_vcgezd_s64:P = llvm::ICmpInst::ICMP_SGE;break;
    case NEON::BI__builtin_neon_vclezd_u64:P = llvm::ICmpInst::ICMP_ULE;break;
    case NEON::BI__builtin_neon_vclezd_s64:P = llvm::ICmpInst::ICMP_SLE;break;
    }
    llvm::Type *Ty = llvm::Type::getInt64Ty(getLLVMContext());
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[0] = Builder.CreateICmp(P, Ops[0], llvm::Constant::getNullValue(Ty));
    return Builder.CreateZExt(Ops[0], Ty, "vceqzd");
  }
  case NEON::BI__builtin_neon_vtstd_s64:
  case NEON::BI__builtin_neon_vtstd_u64: {
    llvm::Type *Ty = llvm::Type::getInt64Ty(getLLVMContext());
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[0] = Builder.CreateAnd(Ops[0], Ops[1]);
    Ops[0] = Builder.CreateICmp(ICmpInst::ICMP_NE, Ops[0],
                                llvm::Constant::getNullValue(Ty));
    return Builder.CreateSExt(Ops[0], Ty, "vtstd");
  }
  case NEON::BI__builtin_neon_vset_lane_i8:
  case NEON::BI__builtin_neon_vset_lane_i16:
  case NEON::BI__builtin_neon_vset_lane_i32:
  case NEON::BI__builtin_neon_vset_lane_i64:
  case NEON::BI__builtin_neon_vset_lane_f16:
  case NEON::BI__builtin_neon_vset_lane_f32:
  case NEON::BI__builtin_neon_vsetq_lane_i8:
  case NEON::BI__builtin_neon_vsetq_lane_i16:
  case NEON::BI__builtin_neon_vsetq_lane_i32:
  case NEON::BI__builtin_neon_vsetq_lane_i64:
  case NEON::BI__builtin_neon_vsetq_lane_f16:
  case NEON::BI__builtin_neon_vsetq_lane_f32:
    Ops.push_back(EmitScalarExpr(E->getArg(2)));
    return Builder.CreateInsertElement(Ops[1], Ops[0], Ops[2], "vset_lane");
  // The code below is a necessary, but not sufficient condition to have a
  // working vset_lane_f64.  Punt for now.  Perhaps replacing Ops[1] with a
  // bitcast from the source to Ops[1] to vif64 would work better?
  // case NEON::BI__builtin_neon_vset_lane_f64:
  //   // The vector type needs a cast for the v1f64 variant.
  //   Ops[1] = Builder.CreateBitCast(Ops[1],
  //       llvm::VectorType::get(llvm::Type::getDoubleTy(getLLVMContext()), 1));
  //   Ops.push_back(EmitScalarExpr(E->getArg(2)));
  //   return Builder.CreateInsertElement(Ops[1], Ops[0], Ops[2], "vset_lane");
  case NEON::BI__builtin_neon_vsetq_lane_f64:
    // The vector type needs a cast for the v2f64 variant.
    Ops[1] = Builder.CreateBitCast(Ops[1],
        llvm::VectorType::get(llvm::Type::getDoubleTy(getLLVMContext()), 2));
    Ops.push_back(EmitScalarExpr(E->getArg(2)));
    return Builder.CreateInsertElement(Ops[1], Ops[0], Ops[2], "vset_lane");

  case NEON::BI__builtin_neon_vcopyq_lane_v: {
    llvm::VectorType *origVTy = GetNeonType(this, Type);
    int elemSize = origVTy->getElementType()->getPrimitiveSizeInBits();
    int numElems = 128/elemSize; // all variants are "q", => V128
    llvm::Type *elemType = llvm::IntegerType::get(getLLVMContext(), elemSize);
    llvm::Type *VTy = llvm::VectorType::get(elemType, numElems);
    llvm::Type *Tys[] = {VTy, VTy};
    Value *intrin = CGM.getIntrinsic(Intrinsic::arm64_neon_vcopy_lane, Tys);
    Ops[1] = ConstantInt::get(Int64Ty,
                              cast<ConstantInt>(Ops[1])->getSExtValue());
    Ops[3] = ConstantInt::get(Int64Ty,
                              cast<ConstantInt>(Ops[3])->getSExtValue());
    return EmitNeonCall((Function*) intrin, Ops, "");
  }
  case NEON::BI__builtin_neon_vget_lane_u8:
  case NEON::BI__builtin_neon_vget_lane_s8:
  case NEON::BI__builtin_neon_vget_lane_p8:
  case NEON::BI__builtin_neon_vdupb_lane_s8:
  case NEON::BI__builtin_neon_vdupb_lane_u8:
    Ops[0] = Builder.CreateBitCast(Ops[0],
        llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 8));
    return Builder.CreateExtractElement(Ops[0], EmitScalarExpr(E->getArg(1)),
                                        "vget_lane");
  case NEON::BI__builtin_neon_vgetq_lane_u8:
  case NEON::BI__builtin_neon_vgetq_lane_s8:
  case NEON::BI__builtin_neon_vgetq_lane_p8:
    Ops[0] = Builder.CreateBitCast(Ops[0],
        llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 16));
    return Builder.CreateExtractElement(Ops[0], EmitScalarExpr(E->getArg(1)),
                                        "vgetq_lane");
  case NEON::BI__builtin_neon_vget_lane_u16:
  case NEON::BI__builtin_neon_vget_lane_s16:
  case NEON::BI__builtin_neon_vget_lane_p16:
  case NEON::BI__builtin_neon_vduph_lane_s16:
  case NEON::BI__builtin_neon_vduph_lane_u16:
    Ops[0] = Builder.CreateBitCast(Ops[0],
        llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 4));
    return Builder.CreateExtractElement(Ops[0], EmitScalarExpr(E->getArg(1)),
                                        "vget_lane");
  case NEON::BI__builtin_neon_vgetq_lane_u16:
  case NEON::BI__builtin_neon_vgetq_lane_s16:
  case NEON::BI__builtin_neon_vgetq_lane_p16:
    Ops[0] = Builder.CreateBitCast(Ops[0],
        llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 8));
    return Builder.CreateExtractElement(Ops[0], EmitScalarExpr(E->getArg(1)),
                                        "vgetq_lane");
  case NEON::BI__builtin_neon_vget_lane_u32:
  case NEON::BI__builtin_neon_vget_lane_s32:
  case NEON::BI__builtin_neon_vdups_lane_s32:
  case NEON::BI__builtin_neon_vdups_lane_u32:
    Ops[0] = Builder.CreateBitCast(Ops[0],
        llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 32), 2));
    return Builder.CreateExtractElement(Ops[0], EmitScalarExpr(E->getArg(1)),
                                        "vget_lane");
  case NEON::BI__builtin_neon_vdups_lane_f32:
    Ops[0] = Builder.CreateBitCast(Ops[0],
        llvm::VectorType::get(llvm::Type::getFloatTy(getLLVMContext()), 2));
    return Builder.CreateExtractElement(Ops[0], EmitScalarExpr(E->getArg(1)),
                                        "vdups_lane");
  case NEON::BI__builtin_neon_vgetq_lane_u32:
  case NEON::BI__builtin_neon_vgetq_lane_s32:
    Ops[0] = Builder.CreateBitCast(Ops[0],
        llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 32), 4));
    return Builder.CreateExtractElement(Ops[0], EmitScalarExpr(E->getArg(1)),
                                        "vgetq_lane");
  case NEON::BI__builtin_neon_vget_lane_u64:
  case NEON::BI__builtin_neon_vget_lane_s64:
  case NEON::BI__builtin_neon_vdupd_lane_s64:
  case NEON::BI__builtin_neon_vdupd_lane_u64:
    Ops[0] = Builder.CreateBitCast(Ops[0],
        llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 64), 1));
    return Builder.CreateExtractElement(Ops[0], EmitScalarExpr(E->getArg(1)),
                                        "vget_lane");
  case NEON::BI__builtin_neon_vdup_lane_f64:
  case NEON::BI__builtin_neon_vdupd_lane_f64:
    Ops[0] = Builder.CreateBitCast(Ops[0],
        llvm::VectorType::get(llvm::Type::getDoubleTy(getLLVMContext()), 1));
    return Builder.CreateExtractElement(Ops[0], EmitScalarExpr(E->getArg(1)),
                                        "vdupd_lane");
  case NEON::BI__builtin_neon_vgetq_lane_u64:
  case NEON::BI__builtin_neon_vgetq_lane_s64:
    Ops[0] = Builder.CreateBitCast(Ops[0],
        llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 64), 2));
    return Builder.CreateExtractElement(Ops[0], EmitScalarExpr(E->getArg(1)),
                                        "vgetq_lane");
  case NEON::BI__builtin_neon_vget_lane_f32:
    Ops[0] = Builder.CreateBitCast(Ops[0],
        llvm::VectorType::get(llvm::Type::getFloatTy(getLLVMContext()), 2));
    return Builder.CreateExtractElement(Ops[0], EmitScalarExpr(E->getArg(1)),
                                        "vget_lane");
  case NEON::BI__builtin_neon_vget_lane_f64:
    Ops[0] = Builder.CreateBitCast(Ops[0],
        llvm::VectorType::get(llvm::Type::getDoubleTy(getLLVMContext()), 1));
    return Builder.CreateExtractElement(Ops[0], EmitScalarExpr(E->getArg(1)),
                                        "vget_lane");
  case NEON::BI__builtin_neon_vgetq_lane_f32:
    Ops[0] = Builder.CreateBitCast(Ops[0],
        llvm::VectorType::get(llvm::Type::getFloatTy(getLLVMContext()), 4));
    return Builder.CreateExtractElement(Ops[0], EmitScalarExpr(E->getArg(1)),
                                        "vgetq_lane");
  case NEON::BI__builtin_neon_vgetq_lane_f64:
    Ops[0] = Builder.CreateBitCast(Ops[0],
        llvm::VectorType::get(llvm::Type::getDoubleTy(getLLVMContext()), 2));
    return Builder.CreateExtractElement(Ops[0], EmitScalarExpr(E->getArg(1)),
                                        "vgetq_lane");
  case NEON::BI__builtin_neon_vabds_f32:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_sisd_fabd,
                                     llvm::Type::getFloatTy(getLLVMContext())),
                        Ops, "vabd");
  case NEON::BI__builtin_neon_vabdd_f64:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_sisd_fabd,
                                     llvm::Type::getDoubleTy(getLLVMContext())),
                        Ops, "vabd");
  case NEON::BI__builtin_neon_vmulxs_f32:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_sisd_fmulx,
                                     llvm::Type::getFloatTy(getLLVMContext())),
                        Ops, "vmulx");
  case NEON::BI__builtin_neon_vmulxd_f64:
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_sisd_fmulx,
                                     llvm::Type::getDoubleTy(getLLVMContext())),
                        Ops, "vmulx");
  case NEON::BI__builtin_neon_vsha1cq_u32: {
    llvm::Type *Int32x4Ty = llvm::VectorType::get(Int32Ty, 4);
    Ops.push_back(EmitScalarExpr(E->getArg(2)));
    Ops[0] = Builder.CreateBitCast(Ops[0], Int32x4Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Int32x4Ty);
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_crypto_sha1c),
                        Ops, "vsha1c");
  }
  case NEON::BI__builtin_neon_vsha1pq_u32: {
    llvm::Type *Int32x4Ty = llvm::VectorType::get(Int32Ty, 4);
    Ops.push_back(EmitScalarExpr(E->getArg(2)));
    Ops[0] = Builder.CreateBitCast(Ops[0], Int32x4Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Int32x4Ty);
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_crypto_sha1p),
                        Ops, "vsha1p");
  }
  case NEON::BI__builtin_neon_vsha1mq_u32: {
    llvm::Type *Int32x4Ty = llvm::VectorType::get(Int32Ty, 4);
    Ops.push_back(EmitScalarExpr(E->getArg(2)));
    Ops[0] = Builder.CreateBitCast(Ops[0], Int32x4Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Int32x4Ty);
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_crypto_sha1m),
                        Ops, "vsha1m");
  }
  case NEON::BI__builtin_neon_vsha1h_u32:
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = Builder.CreateBitCast(Ops[0], Int32Ty);
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_crypto_sha1h),
                        Ops, "vsha1m");
  }

  llvm::VectorType *VTy = GetNeonType(this, Type);
  llvm::Type *Ty = VTy;
  if (!Ty)
    return 0;

  // Not all intrinsics handled by the common case work for ARM64 yet, so only
  // defer to common code if it's been added to our special map.
  unsigned LLVMIntrinsic = 0, AltLLVMIntrinsic = 0;
  bool IsIntrinsicCommon = findNeonIntrinsic(ARM64NeonIntrinsicMap, BuiltinID,
                                             ARM64IntrinsicsProvenSorted,
                                             LLVMIntrinsic, AltLLVMIntrinsic);

  if (IsIntrinsicCommon)
    return EmitCommonNeonBuiltinExpr(BuiltinID, LLVMIntrinsic, AltLLVMIntrinsic,
                                     E, Ops, 0);

  unsigned Int;
  switch (BuiltinID) {
  default: return 0;
  case NEON::BI__builtin_neon_vusqadd_v:
  case NEON::BI__builtin_neon_vusqaddq_v:
    // FIXME: add these intrinsics to arm_neon.h
    Int = Intrinsic::arm64_neon_usqadd;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vusqadd");
  case NEON::BI__builtin_neon_vmulx_lane_v:
  case NEON::BI__builtin_neon_vmulxq_lane_v: { // Only used for FP types
    // FIXME: probably remove this when aarch64_simd.h is no longer supported
    // (arm_neon.h delegates to vmulx without lane).
    llvm::Constant *cst = cast<Constant>(Ops[2]);
    Ops[1] = Builder.CreateBitCast(Ops[1], VTy);
    Ops[1] = EmitNeonSplat(Ops[1], cst);
    Ops.pop_back();
    Int = Intrinsic::arm64_neon_fmulx;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vmulx");
  }
  case NEON::BI__builtin_neon_vfma_lane_v:
  case NEON::BI__builtin_neon_vfmaq_lane_v: { // Only used for FP types
    // The ARM builtins (and instructions) have the addend as the first
    // operand, but the 'fma' intrinsics have it last. Swap it around here.
    Value *Addend = Ops[0];
    Value *Multiplicand = Ops[1];
    Value *LaneSource = Ops[2];
    Ops[0] = Multiplicand;
    Ops[1] = LaneSource;
    Ops[2] = Addend;

    // Now adjust things to handle the lane access.
    llvm::Type *SourceTy = BuiltinID == NEON::BI__builtin_neon_vfmaq_lane_v ?
      llvm::VectorType::get(VTy->getElementType(), VTy->getNumElements() / 2) :
      VTy;
    llvm::Constant *cst = cast<Constant>(Ops[3]);
    Value *SV = llvm::ConstantVector::getSplat(VTy->getNumElements(), cst);
    Ops[1] = Builder.CreateBitCast(Ops[1], SourceTy);
    Ops[1] = Builder.CreateShuffleVector(Ops[1], Ops[1], SV, "lane");

    Ops.pop_back();
    Int = Intrinsic::fma;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "fmla");
  }
  case NEON::BI__builtin_neon_vfms_lane_v:
  case NEON::BI__builtin_neon_vfmsq_lane_v: { // Only used for FP types
    // FIXME: remove this when we no longer support aarch64_simd.h (arm_neon.h
    // delegates to vfma).

    // The ARM builtins (and instructions) have the addend as the first
    // operand, but the 'fma' intrinsics have it last. Swap it around here.
    Value *Minuend = Ops[0];
    Value *Multiplicand = Ops[1];
    Value *LaneSource = Ops[2];
    Ops[0] = Multiplicand;
    Ops[1] = LaneSource;
    Ops[2] = Minuend;
    // Now adjust things to handle the lane access and the negation of
    // one multiplicand so we get a subtract.
    Ops[0] = Builder.CreateBitCast(Ops[0], VTy);
    Ops[0] = Builder.CreateFNeg(Ops[0]);

    llvm::Type *SourceTy = BuiltinID == NEON::BI__builtin_neon_vfmsq_lane_v ?
      llvm::VectorType::get(VTy->getElementType(), VTy->getNumElements() / 2) :
      VTy;
    llvm::Constant *cst = cast<Constant>(Ops[3]);
    Value *SV = llvm::ConstantVector::getSplat(VTy->getNumElements(), cst);
    Ops[1] = Builder.CreateBitCast(Ops[1], SourceTy);
    Ops[1] = Builder.CreateShuffleVector(Ops[1], Ops[1], SV, "lane");
    Ops[1] = Builder.CreateBitCast(Ops[1], VTy);
    Ops[1] = EmitNeonSplat(Ops[1], cst);

    Ops.pop_back();
    Int = Intrinsic::fma;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "fmls");
  }
  case NEON::BI__builtin_neon_vfms_v:
  case NEON::BI__builtin_neon_vfmsq_v: {  // Only used for FP types
    // FIXME: probably remove when we no longer support aarch64_simd.h
    // (arm_neon.h delegates to vfma).

    // The ARM builtins (and instructions) have the addend as the first
    // operand, but the 'fma' intrinsics have it last. Swap it around here.
    Value *Subtrahend = Ops[0];
    Value *Multiplicand = Ops[2];
    Ops[0] = Multiplicand;
    Ops[2] = Subtrahend;
    Ops[1] = Builder.CreateBitCast(Ops[1], VTy);
    Ops[1] = Builder.CreateFNeg(Ops[1]);
    Int = Intrinsic::fma;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "fmls");
  }
  case NEON::BI__builtin_neon_vmull_v:
    // FIXME: improve sharing scheme to cope with 3 alternative LLVM intrinsics.
    Int = usgn ? Intrinsic::arm64_neon_umull : Intrinsic::arm64_neon_smull;
    Int = Type.isPoly() ? Intrinsic::arm64_neon_pmull : Int;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vmull");
  case NEON::BI__builtin_neon_vmax_v:
  case NEON::BI__builtin_neon_vmaxq_v:
    // FIXME: improve sharing scheme to cope with 3 alternative LLVM intrinsics.
    Int = usgn ? Intrinsic::arm64_neon_umax : Intrinsic::arm64_neon_smax;
    if (Ty->isFPOrFPVectorTy()) Int = Intrinsic::arm64_neon_fmax;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vmax");
  case NEON::BI__builtin_neon_vmin_v:
  case NEON::BI__builtin_neon_vminq_v:
    // FIXME: improve sharing scheme to cope with 3 alternative LLVM intrinsics.
    Int = usgn ? Intrinsic::arm64_neon_umin : Intrinsic::arm64_neon_smin;
    if (Ty->isFPOrFPVectorTy()) Int = Intrinsic::arm64_neon_fmin;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vmin");
  case NEON::BI__builtin_neon_vabd_v:
  case NEON::BI__builtin_neon_vabdq_v:
    // FIXME: improve sharing scheme to cope with 3 alternative LLVM intrinsics.
    Int = usgn ? Intrinsic::arm64_neon_uabd : Intrinsic::arm64_neon_sabd;
    if (Ty->isFPOrFPVectorTy()) Int = Intrinsic::arm64_neon_fabd;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vabd");
  case NEON::BI__builtin_neon_vabdl_v:
    // FIXME: probably remove when aarch64_simd.h is no longer supported
    // (arm_neon.h uses vabd + vmovl).
    Int = usgn ? Intrinsic::arm64_neon_uabdl : Intrinsic::arm64_neon_sabdl;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vabdl");
  case NEON::BI__builtin_neon_vabal_v: {
    // FIXME: probably remove when aarch64_simd.h is no longer supported
    // (arm_neon.h delegates to vabdl and addition).
    Int = usgn ? Intrinsic::arm64_neon_uabdl : Intrinsic::arm64_neon_sabdl;
    SmallVector<llvm::Value*, 2> TmpOps;
    TmpOps.push_back(Ops[1]);
    TmpOps.push_back(Ops[2]);
    llvm::Value *tmp =
        EmitNeonCall(CGM.getIntrinsic(Int, Ty), TmpOps, "vabdl");
    llvm::Value *addend = Builder.CreateBitCast(Ops[0], Ty);
    return Builder.CreateAdd(addend, tmp);
  }
  case NEON::BI__builtin_neon_vpadal_v:
  case NEON::BI__builtin_neon_vpadalq_v: {
    unsigned ArgElts = VTy->getNumElements();
    llvm::IntegerType *EltTy = cast<IntegerType>(VTy->getElementType());
    unsigned BitWidth = EltTy->getBitWidth();
    llvm::Type *ArgTy = llvm::VectorType::get(
        llvm::IntegerType::get(getLLVMContext(), BitWidth/2), 2*ArgElts);
    llvm::Type* Tys[2] = { VTy, ArgTy };
    Int = usgn ? Intrinsic::arm64_neon_uaddlp : Intrinsic::arm64_neon_saddlp;
    SmallVector<llvm::Value*, 1> TmpOps;
    TmpOps.push_back(Ops[1]);
    Function *F = CGM.getIntrinsic(Int, Tys);
    llvm::Value *tmp = EmitNeonCall(F, TmpOps, "vpadal");
    llvm::Value *addend = Builder.CreateBitCast(Ops[0], tmp->getType());
    return Builder.CreateAdd(tmp, addend);
  }
  case NEON::BI__builtin_neon_vpmin_v:
  case NEON::BI__builtin_neon_vpminq_v:
    // FIXME: improve sharing scheme to cope with 3 alternative LLVM intrinsics.
    Int = usgn ? Intrinsic::arm64_neon_uminp : Intrinsic::arm64_neon_sminp;
    if (Ty->isFPOrFPVectorTy()) Int = Intrinsic::arm64_neon_fminp;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vpmin");
  case NEON::BI__builtin_neon_vpmax_v:
  case NEON::BI__builtin_neon_vpmaxq_v:
    // FIXME: improve sharing scheme to cope with 3 alternative LLVM intrinsics.
    Int = usgn ? Intrinsic::arm64_neon_umaxp : Intrinsic::arm64_neon_smaxp;
    if (Ty->isFPOrFPVectorTy()) Int = Intrinsic::arm64_neon_fmaxp;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vpmax");
  case NEON::BI__builtin_neon_vminnm_v:
  case NEON::BI__builtin_neon_vminnmq_v:
    Int = Intrinsic::arm64_neon_fminnm;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vminnm");
  case NEON::BI__builtin_neon_vmaxnm_v:
  case NEON::BI__builtin_neon_vmaxnmq_v:
    Int = Intrinsic::arm64_neon_fmaxnm;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vmaxnm");
  case NEON::BI__builtin_neon_vrecpss_f32: {
    llvm::Type *f32Type = llvm::Type::getFloatTy(getLLVMContext());
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_sisd_recp, f32Type),
                        Ops, "vrecps");
  }
  case NEON::BI__builtin_neon_vrecpsd_f64: {
    llvm::Type *f64Type = llvm::Type::getDoubleTy(getLLVMContext());
    Ops.push_back(EmitScalarExpr(E->getArg(1)));
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_sisd_recp, f64Type),
                        Ops, "vrecps");
  }
  case NEON::BI__builtin_neon_vrshr_n_v:
  case NEON::BI__builtin_neon_vrshrq_n_v:
    // FIXME: this can be shared with 32-bit ARM, but not AArch64 at the
    // moment. After the final merge it should be added to
    // EmitCommonNeonBuiltinExpr.
    Int = usgn ? Intrinsic::arm64_neon_urshl : Intrinsic::arm64_neon_srshl;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrshr_n", 1, true);
  case NEON::BI__builtin_neon_vqshlu_n_v:
  case NEON::BI__builtin_neon_vqshluq_n_v:
    // FIXME: AArch64 and ARM use different intrinsics for this, but are
    // essentially compatible. It should be in EmitCommonNeonBuiltinExpr after
    // the final merge.
    Int = Intrinsic::arm64_neon_sqshlu;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqshlu_n", 1, false);
  case NEON::BI__builtin_neon_vqshrun_n_v:
    // FIXME: as above
    Int = Intrinsic::arm64_neon_sqshrun;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqshrun_n");
  case NEON::BI__builtin_neon_vqrshrun_n_v:
    // FIXME: and again.
    Int = Intrinsic::arm64_neon_sqrshrun;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqrshrun_n");
  case NEON::BI__builtin_neon_vqshrn_n_v:
    // FIXME: guess
    Int = usgn ? Intrinsic::arm64_neon_uqshrn : Intrinsic::arm64_neon_sqshrn;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqshrn_n");
  case NEON::BI__builtin_neon_vrshrn_n_v:
    // FIXME: there might be a pattern here.
    Int = Intrinsic::arm64_neon_rshrn;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrshrn_n");
  case NEON::BI__builtin_neon_vqrshrn_n_v:
    // FIXME: another one
    Int = usgn ? Intrinsic::arm64_neon_uqrshrn : Intrinsic::arm64_neon_sqrshrn;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqrshrn_n");
  case NEON::BI__builtin_neon_vshll_n_v: {
    llvm::Type *SrcTy = llvm::VectorType::getTruncatedElementVectorType(VTy);
    Ops[0] = Builder.CreateBitCast(Ops[0], SrcTy);
    if (usgn)
      Ops[0] = Builder.CreateZExt(Ops[0], VTy);
    else
      Ops[0] = Builder.CreateSExt(Ops[0], VTy);
    Ops[1] = EmitNeonShiftVector(Ops[1], VTy, false);
    return Builder.CreateShl(Ops[0], Ops[1], "vshll_n");
  }
  case NEON::BI__builtin_neon_vrnda_v:
  case NEON::BI__builtin_neon_vrndaq_v: {
    Int = Intrinsic::arm64_neon_frinta;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrnda");
  }
  case NEON::BI__builtin_neon_vrndm_v:
  case NEON::BI__builtin_neon_vrndmq_v: {
    Int = Intrinsic::arm64_neon_frintm;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrndm");
  }
  case NEON::BI__builtin_neon_vrndn_v:
  case NEON::BI__builtin_neon_vrndnq_v: {
    Int = Intrinsic::arm64_neon_frintn;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrndn");
  }
  case NEON::BI__builtin_neon_vrndp_v:
  case NEON::BI__builtin_neon_vrndpq_v: {
    Int = Intrinsic::arm64_neon_frintp;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrndp");
  }
  case NEON::BI__builtin_neon_vrndx_v:
  case NEON::BI__builtin_neon_vrndxq_v: {
    Int = Intrinsic::arm64_neon_frintx;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrndx");
  }
  case NEON::BI__builtin_neon_vrndz_v:
  case NEON::BI__builtin_neon_vrnd_v:
  case NEON::BI__builtin_neon_vrndzq_v:
  case NEON::BI__builtin_neon_vrndq_v: {
    Int = Intrinsic::arm64_neon_frintz;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrndz");
  }
  case NEON::BI__builtin_neon_vcvt_f32_v:
  case NEON::BI__builtin_neon_vcvtq_f32_v:
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ty = GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float32, false, quad));
    return usgn ? Builder.CreateUIToFP(Ops[0], Ty, "vcvt")
                : Builder.CreateSIToFP(Ops[0], Ty, "vcvt");
  case NEON::BI__builtin_neon_vcvtq_f64_v:
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ty = GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float64, false, true));
    return usgn ? Builder.CreateUIToFP(Ops[0], Ty, "vcvt")
                : Builder.CreateSIToFP(Ops[0], Ty, "vcvt");
  case NEON::BI__builtin_neon_vcvt_f32_f16: {
    assert(Type.getEltType() == NeonTypeFlags::Float16 && !quad &&
           "unexpected vcvt_f32_f16 builtin");
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_vcvthf2fp);
    return EmitNeonCall(F, Ops, "vcvt");
  }
  case NEON::BI__builtin_neon_vcvt_f64_f32: {
    assert(Type.getEltType() == NeonTypeFlags::Float64 && quad &&
           "unexpected vcvt_f64_f32 builtin");
    NeonTypeFlags SrcFlag = NeonTypeFlags(NeonTypeFlags::Float32, false, false);
    Ops[0] = Builder.CreateBitCast(Ops[0], GetNeonType(this, SrcFlag));

    return Builder.CreateFPExt(Ops[0], Ty, "vcvt");
  }
  case NEON::BI__builtin_neon_vcvt_high_f64_f32: {
    assert(Type.getEltType() == NeonTypeFlags::Float64 && quad &&
           "unexpected vcvt_high_f64_f32 builtin");
    NeonTypeFlags SrcFlag = NeonTypeFlags(NeonTypeFlags::Float32, false, true);
    Ops[0] = Builder.CreateBitCast(Ops[0], GetNeonType(this, SrcFlag));

    SmallVector<Constant*, 2> Indices;
    for (unsigned i = 2; i < 4; ++i)
      Indices.push_back(ConstantInt::get(Int32Ty, i));
    Constant *Mask = ConstantVector::get(Indices);
    Value *HighVec = Builder.CreateShuffleVector(Ops[0], Ops[0], Mask);

    return Builder.CreateFPExt(HighVec, Ty, "vcvt");
  }
  case NEON::BI__builtin_neon_vcvt_f32_f64: {
    assert(Type.getEltType() == NeonTypeFlags::Float32 &&
           "unexpected vcvt_f32_f64 builtin");
    NeonTypeFlags SrcFlag = NeonTypeFlags(NeonTypeFlags::Float64, false, true);
    Ops[0] = Builder.CreateBitCast(Ops[0], GetNeonType(this, SrcFlag));

    return Builder.CreateFPTrunc(Ops[0], Ty, "vcvt");
  }
  case NEON::BI__builtin_neon_vcvt_high_f32_f64: {
    assert(Type.getEltType() == NeonTypeFlags::Float32 && quad &&
           "unexpected vcvt_high_f32_f64 builtin");
    NeonTypeFlags SrcFlag = NeonTypeFlags(NeonTypeFlags::Float64, false, true);
    Ops[1] = Builder.CreateBitCast(Ops[1], GetNeonType(this, SrcFlag));

    llvm::Type *TruncTy =
        GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float32, false, false));
    Value *Trunc = Builder.CreateFPTrunc(Ops[1], TruncTy, "vcvt");

    return EmitConcatVectors(Ops[0], Trunc, TruncTy);
  }
  case NEON::BI__builtin_neon_vcvtx_f32_f64: {
    assert(Type.getEltType() == NeonTypeFlags::Float32 &&
           "unexpected vcvtx_f32_f64 builtin");
    llvm::Type *DblTy =
      GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float64, false, true));
    llvm::Type *Tys[2] = { Ty, DblTy };
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_fcvtxn, Tys);
    return EmitNeonCall(F, Ops, "vcvtx");
  }
  case NEON::BI__builtin_neon_vcvtx_high_f32_f64: {
    assert(Type.getEltType() == NeonTypeFlags::Float32 && quad &&
           "unexpected vcvtx_high_f32_f64 builtin");
    llvm::Type *DblTy =
      GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float64, false, true));
    llvm::Type *FltTy =
      GetNeonType(this, NeonTypeFlags(NeonTypeFlags::Float32, false, false));
    llvm::Type *Tys[2] = { FltTy, DblTy };
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_fcvtxn, Tys);

    SmallVector<Value*, 1> IntOps;
    IntOps.push_back(Ops[1]);
    Value *CvtVal = EmitNeonCall(F, IntOps, "vcvtx");

    return EmitConcatVectors(Ops[0], CvtVal, FltTy);
  }
  case NEON::BI__builtin_neon_vcvt_f16_v: {
    assert(Type.getEltType() == NeonTypeFlags::Float16 && !quad &&
           "unexpected vcvt_f16_v builtin");
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_vcvtfp2hf);
    return EmitNeonCall(F, Ops, "vcvt");
  }
  case NEON::BI__builtin_neon_vcvt_n_f32_v:
  case NEON::BI__builtin_neon_vcvtq_n_f32_v:
  case NEON::BI__builtin_neon_vcvtq_n_f64_v: {
    bool Double =
      (cast<llvm::IntegerType>(VTy->getElementType())->getBitWidth() == 64);
    llvm::Type *FloatTy =
      GetNeonType(this,
                  NeonTypeFlags(Double ? NeonTypeFlags::Float64
                                : NeonTypeFlags::Float32, false, quad));
    llvm::Type *Tys[2] = { FloatTy, Ty };
    Int = usgn ? Intrinsic::arm64_neon_vcvtfxu2fp
               : Intrinsic::arm64_neon_vcvtfxs2fp;
    Function *F = CGM.getIntrinsic(Int, Tys);
    return EmitNeonCall(F, Ops, "vcvt_n");
  }
  case NEON::BI__builtin_neon_vcvt_n_s32_v:
  case NEON::BI__builtin_neon_vcvtq_n_s32_v:
  case NEON::BI__builtin_neon_vcvtq_n_s64_v: {
    bool Double =
      (cast<llvm::IntegerType>(VTy->getElementType())->getBitWidth() == 64);
    llvm::Type *FloatTy =
      GetNeonType(this,
                  NeonTypeFlags(Double ? NeonTypeFlags::Float64
                                : NeonTypeFlags::Float32, false, quad));
    llvm::Type *Tys[2] = { Ty, FloatTy };
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_vcvtfp2fxs, Tys);
    return EmitNeonCall(F, Ops, "vcvt_n");
  }
  case NEON::BI__builtin_neon_vcvt_n_u32_v:
  case NEON::BI__builtin_neon_vcvtq_n_u32_v:
  case NEON::BI__builtin_neon_vcvtq_n_u64_v: {
    bool Double =
      (cast<llvm::IntegerType>(VTy->getElementType())->getBitWidth() == 64);
    llvm::Type *FloatTy =
      GetNeonType(this,
                  NeonTypeFlags(Double ? NeonTypeFlags::Float64
                                : NeonTypeFlags::Float32, false, quad));
    llvm::Type *Tys[2] = { Ty, FloatTy };
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_vcvtfp2fxu, Tys);
    return EmitNeonCall(F, Ops, "vcvt_n");
  }
  case NEON::BI__builtin_neon_vcvt_s32_v:
  case NEON::BI__builtin_neon_vcvt_u32_v:
  case NEON::BI__builtin_neon_vcvtq_s32_v:
  case NEON::BI__builtin_neon_vcvtq_u32_v:
  case NEON::BI__builtin_neon_vcvtq_s64_v:
  case NEON::BI__builtin_neon_vcvtq_u64_v:
  case NEON::BI__builtin_neon_vcvtz_s32_v:
  case NEON::BI__builtin_neon_vcvtzq_s32_v:
  case NEON::BI__builtin_neon_vcvtz_u32_v:
  case NEON::BI__builtin_neon_vcvtzq_u32_v:
  case NEON::BI__builtin_neon_vcvtzq_s64_v:
  case NEON::BI__builtin_neon_vcvtzq_u64_v: {
    bool Double =
      (cast<llvm::IntegerType>(VTy->getElementType())->getBitWidth() == 64);
    llvm::Type *InTy =
      GetNeonType(this,
                  NeonTypeFlags(Double ? NeonTypeFlags::Float64
                                : NeonTypeFlags::Float32, false, quad));
    Ops[0] = Builder.CreateBitCast(Ops[0], InTy);
    if (usgn)
      return Builder.CreateFPToUI(Ops[0], Ty);
    return Builder.CreateFPToSI(Ops[0], Ty);
  }
  case NEON::BI__builtin_neon_vcvta_s32_v:
  case NEON::BI__builtin_neon_vcvtaq_s32_v:
  case NEON::BI__builtin_neon_vcvta_u32_v:
  case NEON::BI__builtin_neon_vcvtaq_u32_v:
  case NEON::BI__builtin_neon_vcvtaq_s64_v:
  case NEON::BI__builtin_neon_vcvtaq_u64_v: {
    Int = usgn ? Intrinsic::arm64_neon_fcvtau : Intrinsic::arm64_neon_fcvtas;
    bool Double =
      (cast<llvm::IntegerType>(VTy->getElementType())->getBitWidth() == 64);
    llvm::Type *InTy =
      GetNeonType(this,
                  NeonTypeFlags(Double ? NeonTypeFlags::Float64
                                : NeonTypeFlags::Float32, false, quad));
    llvm::Type *Tys[2] = { Ty, InTy };
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvta");
  }
  case NEON::BI__builtin_neon_vcvtm_s32_v:
  case NEON::BI__builtin_neon_vcvtmq_s32_v:
  case NEON::BI__builtin_neon_vcvtm_u32_v:
  case NEON::BI__builtin_neon_vcvtmq_u32_v:
  case NEON::BI__builtin_neon_vcvtmq_s64_v:
  case NEON::BI__builtin_neon_vcvtmq_u64_v: {
    Int = usgn ? Intrinsic::arm64_neon_fcvtmu : Intrinsic::arm64_neon_fcvtms;
    bool Double =
      (cast<llvm::IntegerType>(VTy->getElementType())->getBitWidth() == 64);
    llvm::Type *InTy =
      GetNeonType(this,
                  NeonTypeFlags(Double ? NeonTypeFlags::Float64
                                : NeonTypeFlags::Float32, false, quad));
    llvm::Type *Tys[2] = { Ty, InTy };
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtm");
  }
  case NEON::BI__builtin_neon_vcvtn_s32_v:
  case NEON::BI__builtin_neon_vcvtnq_s32_v:
  case NEON::BI__builtin_neon_vcvtn_u32_v:
  case NEON::BI__builtin_neon_vcvtnq_u32_v:
  case NEON::BI__builtin_neon_vcvtnq_s64_v:
  case NEON::BI__builtin_neon_vcvtnq_u64_v: {
    Int = usgn ? Intrinsic::arm64_neon_fcvtnu : Intrinsic::arm64_neon_fcvtns;
    bool Double =
      (cast<llvm::IntegerType>(VTy->getElementType())->getBitWidth() == 64);
    llvm::Type *InTy =
      GetNeonType(this,
                  NeonTypeFlags(Double ? NeonTypeFlags::Float64
                                : NeonTypeFlags::Float32, false, quad));
    llvm::Type *Tys[2] = { Ty, InTy };
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtn");
  }
  case NEON::BI__builtin_neon_vcvtp_s32_v:
  case NEON::BI__builtin_neon_vcvtpq_s32_v:
  case NEON::BI__builtin_neon_vcvtp_u32_v:
  case NEON::BI__builtin_neon_vcvtpq_u32_v:
  case NEON::BI__builtin_neon_vcvtpq_s64_v:
  case NEON::BI__builtin_neon_vcvtpq_u64_v: {
    Int = usgn ? Intrinsic::arm64_neon_fcvtpu : Intrinsic::arm64_neon_fcvtps;
    bool Double =
      (cast<llvm::IntegerType>(VTy->getElementType())->getBitWidth() == 64);
    llvm::Type *InTy =
      GetNeonType(this,
                  NeonTypeFlags(Double ? NeonTypeFlags::Float64
                                : NeonTypeFlags::Float32, false, quad));
    llvm::Type *Tys[2] = { Ty, InTy };
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vcvtp");
  }
  case NEON::BI__builtin_neon_vdiv_v:
  case NEON::BI__builtin_neon_vdivq_v:
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    return Builder.CreateFDiv(Ops[0], Ops[1]);
  case NEON::BI__builtin_neon_vmulx_v:
  case NEON::BI__builtin_neon_vmulxq_v: {
    Int = Intrinsic::arm64_neon_fmulx;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vmulx");
  }
  case NEON::BI__builtin_neon_vpmaxnm_v:
  case NEON::BI__builtin_neon_vpmaxnmq_v: {
    Int = Intrinsic::arm64_neon_fmaxnmp;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vpmaxnm");
  }
  case NEON::BI__builtin_neon_vpminnm_v:
  case NEON::BI__builtin_neon_vpminnmq_v: {
    Int = Intrinsic::arm64_neon_fminnmp;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vpminnm");
  }
  case NEON::BI__builtin_neon_vsqrt_v:
  case NEON::BI__builtin_neon_vsqrtq_v: {
    Int = Intrinsic::sqrt;
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vsqrt");
  }
  case NEON::BI__builtin_neon_vrbit_v:
  case NEON::BI__builtin_neon_vrbitq_v: {
    Int = Intrinsic::arm64_neon_rbit;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vrbit");
  }
  case NEON::BI__builtin_neon_vaddv_u8:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vaddv_s8: {
    Int = usgn ? Intrinsic::arm64_neon_uaddv : Intrinsic::arm64_neon_saddv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 8);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 8));
  }
  case NEON::BI__builtin_neon_vaddv_u16:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vaddv_s16: {
    Int = usgn ? Intrinsic::arm64_neon_uaddv : Intrinsic::arm64_neon_saddv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 16));
  }
  case NEON::BI__builtin_neon_vaddv_u32:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vaddv_s32: {
    Int = usgn ? Intrinsic::arm64_neon_uaddv : Intrinsic::arm64_neon_saddv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 32), 2);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 32));
  }
  case NEON::BI__builtin_neon_vaddvq_u8:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vaddvq_s8: {
    Int = usgn ? Intrinsic::arm64_neon_uaddv : Intrinsic::arm64_neon_saddv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 16);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 8));
  }
  case NEON::BI__builtin_neon_vaddvq_u16:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vaddvq_s16: {
    Int = usgn ? Intrinsic::arm64_neon_uaddv : Intrinsic::arm64_neon_saddv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 8);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 16));
  }
  case NEON::BI__builtin_neon_vaddvq_u32:
    usgn = true;
    // FALLTHROUGH
  case NEON::BI__builtin_neon_vaddvq_s32: {
    Int = usgn ? Intrinsic::arm64_neon_uaddv : Intrinsic::arm64_neon_saddv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy = llvm::VectorType::get(Ty, 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddv");
  }
  case NEON::BI__builtin_neon_vmaxv_u8: {
    Int = Intrinsic::arm64_neon_umaxv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 8);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 8));
  }
  case NEON::BI__builtin_neon_vmaxv_u16: {
    Int = Intrinsic::arm64_neon_umaxv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 16));
  }
  case NEON::BI__builtin_neon_vmaxv_f32: {
    Int = Intrinsic::arm64_neon_fmaxv; // will transform into fmaxp
    Ty = llvm::Type::getFloatTy(getLLVMContext());
    VTy = llvm::VectorType::get(Ty, 2);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxv");
  }
  case NEON::BI__builtin_neon_vmaxv_u32: {
    Int = Intrinsic::arm64_neon_umaxv; // will transform into umaxp
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 32), 2);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 32));
  }
  case NEON::BI__builtin_neon_vmaxvq_u8: {
    Int = Intrinsic::arm64_neon_umaxv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 16);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 8));
  }
  case NEON::BI__builtin_neon_vmaxvq_u16: {
    Int = Intrinsic::arm64_neon_umaxv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 8);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 16));
  }
  case NEON::BI__builtin_neon_vmaxvq_u32: {
    Int = Intrinsic::arm64_neon_umaxv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy = llvm::VectorType::get(Ty, 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxv");
  }
  case NEON::BI__builtin_neon_vmaxv_s8: {
    Int = Intrinsic::arm64_neon_smaxv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 8);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 8));
  }
  case NEON::BI__builtin_neon_vmaxv_s16: {
    Int = Intrinsic::arm64_neon_smaxv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 16));
  }
  case NEON::BI__builtin_neon_vmaxv_s32: {
    Int = Intrinsic::arm64_neon_smaxv; // will transform into smaxp
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 32), 2);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 32));
  }
  case NEON::BI__builtin_neon_vmaxvq_s8: {
    Int = Intrinsic::arm64_neon_smaxv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 16);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 8));
  }
  case NEON::BI__builtin_neon_vmaxvq_s16: {
    Int = Intrinsic::arm64_neon_smaxv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 8);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 16));
  }
  case NEON::BI__builtin_neon_vmaxvq_s32: {
    Int = Intrinsic::arm64_neon_smaxv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy = llvm::VectorType::get(Ty, 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxv");
  }
  case NEON::BI__builtin_neon_vmaxvq_f32: {
    Int = Intrinsic::arm64_neon_fmaxv;
    Ty = llvm::Type::getFloatTy(getLLVMContext());
    VTy = llvm::VectorType::get(Ty, 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxv");
  }
  case NEON::BI__builtin_neon_vmaxvq_f64: {
    Int = Intrinsic::arm64_neon_fmaxv;  // convert to fmaxp later
    Ty = llvm::Type::getDoubleTy(getLLVMContext());
    VTy = llvm::VectorType::get(Ty, 2);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxv");
  }
  case NEON::BI__builtin_neon_vminv_u8: {
    Int = Intrinsic::arm64_neon_uminv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 8);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 8));
  }
  case NEON::BI__builtin_neon_vminv_u16: {
    Int = Intrinsic::arm64_neon_uminv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 16));
  }
  case NEON::BI__builtin_neon_vminv_u32: {
    Int = Intrinsic::arm64_neon_uminv; // will transform into uminp
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 32), 2);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 32));
  }
  case NEON::BI__builtin_neon_vminvq_u8: {
    Int = Intrinsic::arm64_neon_uminv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 16);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 8));
  }
  case NEON::BI__builtin_neon_vminv_s32: {
    Int = Intrinsic::arm64_neon_sminv; // will transform into sminp
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 32), 2);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 32));
  }
  case NEON::BI__builtin_neon_vminv_f32: {
    Int = Intrinsic::arm64_neon_fminv; // will transform into fminp
    Ty = llvm::Type::getFloatTy(getLLVMContext());
    VTy = llvm::VectorType::get(Ty, 2);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminv");
  }
  case NEON::BI__builtin_neon_vminvq_u16: {
    Int = Intrinsic::arm64_neon_uminv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 8);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 16));
  }
  case NEON::BI__builtin_neon_vminvq_u32: {
    Int = Intrinsic::arm64_neon_uminv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 32), 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminv");
  }
  case NEON::BI__builtin_neon_vminv_s8: {
    Int = Intrinsic::arm64_neon_sminv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 8);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 8));
  }
  case NEON::BI__builtin_neon_vminv_s16: {
    Int = Intrinsic::arm64_neon_sminv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 16));
  }
  case NEON::BI__builtin_neon_vminvq_s8: {
    Int = Intrinsic::arm64_neon_sminv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 16);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 8));
  }
  case NEON::BI__builtin_neon_vminvq_s16: {
    Int = Intrinsic::arm64_neon_sminv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 8);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 16));
  }
  case NEON::BI__builtin_neon_vminvq_s32: {
    Int = Intrinsic::arm64_neon_sminv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy = llvm::VectorType::get(Ty, 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminv");
  }
  case NEON::BI__builtin_neon_vminvq_f32: {
    Int = Intrinsic::arm64_neon_fminv;
    Ty = llvm::Type::getFloatTy(getLLVMContext());
    VTy = llvm::VectorType::get(Ty, 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminv");
  }
  case NEON::BI__builtin_neon_vminvq_f64: {
    Int = Intrinsic::arm64_neon_fminv;  // convert to fminp later
    Ty = llvm::Type::getDoubleTy(getLLVMContext());
    VTy = llvm::VectorType::get(Ty, 2);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    llvm::Function *intrin = CGM.getIntrinsic(Int, Tys);
    return EmitNeonCall(intrin, Ops, "vminv");
  }
  case NEON::BI__builtin_neon_vaddlv_u8: {
    Int = Intrinsic::arm64_neon_uaddlv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 8);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddlv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 16));
  }
  case NEON::BI__builtin_neon_vaddlv_u16: {
    Int = Intrinsic::arm64_neon_uaddlv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddlv");
  }
  case NEON::BI__builtin_neon_vaddlv_u32: {  // Need truncate?
    Int = Intrinsic::arm64_neon_uaddlv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 64);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 32), 2);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddlv");
  }
  case NEON::BI__builtin_neon_vaddlvq_u8: {
    Int = Intrinsic::arm64_neon_uaddlv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 16);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddlv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 16));
  }
  case NEON::BI__builtin_neon_vaddlvq_u16: {
    Int = Intrinsic::arm64_neon_uaddlv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 8);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddlv");
  }
  case NEON::BI__builtin_neon_vaddlvq_u32: {
    Int = Intrinsic::arm64_neon_uaddlv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 64);
    VTy = llvm::VectorType::get(IntegerType::get(getLLVMContext(), 32), 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddlv");
  }
  case NEON::BI__builtin_neon_vaddlv_s8: {
    Int = Intrinsic::arm64_neon_saddlv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 8);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddlv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 16));
  }
  case NEON::BI__builtin_neon_vaddlv_s16: {
    Int = Intrinsic::arm64_neon_saddlv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddlv");
  }
  case NEON::BI__builtin_neon_vaddlv_s32: {  // Need Truncate?
    Int = Intrinsic::arm64_neon_saddlv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 64);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 32), 2);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddlv");
  }
  case NEON::BI__builtin_neon_vaddlvq_s8: {
    Int = Intrinsic::arm64_neon_saddlv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 8), 16);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    Ops[0] = EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddlv");
    return Builder.CreateTrunc(Ops[0],
             llvm::IntegerType::get(getLLVMContext(), 16));
  }
  case NEON::BI__builtin_neon_vaddlvq_s16: {
    Int = Intrinsic::arm64_neon_saddlv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 32);
    VTy =
      llvm::VectorType::get(llvm::IntegerType::get(getLLVMContext(), 16), 8);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddlv");
  }
  case NEON::BI__builtin_neon_vaddlvq_s32: {
    Int = Intrinsic::arm64_neon_saddlv;
    Ty = llvm::IntegerType::get(getLLVMContext(), 64);
    VTy = llvm::VectorType::get(IntegerType::get(getLLVMContext(), 32), 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vaddlv");
  }
  case NEON::BI__builtin_neon_vmaxnmv_f32: {
    Int = Intrinsic::arm64_neon_fmaxnmv;  // convert to fmaxnmp later
    Ty = llvm::Type::getFloatTy(getLLVMContext());
    VTy = llvm::VectorType::get(Ty, 2);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxnmv");
  }
  case NEON::BI__builtin_neon_vmaxnmvq_f32: {
    Int = Intrinsic::arm64_neon_fmaxnmv;
    Ty = llvm::Type::getFloatTy(getLLVMContext());
    VTy = llvm::VectorType::get(Ty, 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vmaxnmv");
  }
  case NEON::BI__builtin_neon_vminnmv_f32: {
    Int = Intrinsic::arm64_neon_fminnmv;  // convert to fminnmp later
    Ty = llvm::Type::getFloatTy(getLLVMContext());
    VTy = llvm::VectorType::get(Ty, 2);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminnmv");
  }
  case NEON::BI__builtin_neon_vminnmvq_f32: {
    Int = Intrinsic::arm64_neon_fminnmv;
    Ty = llvm::Type::getFloatTy(getLLVMContext());
    VTy = llvm::VectorType::get(Ty, 4);
    llvm::Type *Tys[2] = { Ty, VTy };
    Ops.push_back(EmitScalarExpr(E->getArg(0)));
    return EmitNeonCall(CGM.getIntrinsic(Int, Tys), Ops, "vminnmv");
  }
  case NEON::BI__builtin_neon_vsri_n_v:
  case NEON::BI__builtin_neon_vsriq_n_v: {
    Int = Intrinsic::arm64_neon_vsri;
    llvm::Function *Intrin = CGM.getIntrinsic(Int, Ty); 
    return EmitNeonCall(Intrin, Ops, "vsri_n");
  }
  case NEON::BI__builtin_neon_vsli_n_v:
  case NEON::BI__builtin_neon_vsliq_n_v: {
    Int = Intrinsic::arm64_neon_vsli;
    llvm::Function *Intrin = CGM.getIntrinsic(Int, Ty); 
    return EmitNeonCall(Intrin, Ops, "vsli_n");
  }
  case NEON::BI__builtin_neon_vsra_n_v:
  case NEON::BI__builtin_neon_vsraq_n_v:
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[2] = EmitNeonShiftVector(Ops[2], Ty, false);
    if (usgn)
      Ops[1] = Builder.CreateLShr(Ops[1], Ops[2], "vsra_n");
    else
      Ops[1] = Builder.CreateAShr(Ops[1], Ops[2], "vsra_n");
    return Builder.CreateAdd(Ops[0], Ops[1]);
  case NEON::BI__builtin_neon_vrsra_n_v:
  case NEON::BI__builtin_neon_vrsraq_n_v: {
    Int = usgn ? Intrinsic::arm64_neon_urshl : Intrinsic::arm64_neon_srshl;
    SmallVector<llvm::Value*,2> TmpOps;
    TmpOps.push_back(Ops[1]);
    TmpOps.push_back(Ops[2]);
    Function* F = CGM.getIntrinsic(Int, Ty);
    llvm::Value *tmp = EmitNeonCall(F, TmpOps, "vrshr_n", 1, true);
    Ops[0] = Builder.CreateBitCast(Ops[0], VTy);
    return Builder.CreateAdd(Ops[0], tmp);
  }
  case NEON::BI__builtin_neon_vld1_v:
  case NEON::BI__builtin_neon_vld1q_v:
    Ops[0] = Builder.CreateBitCast(Ops[0], llvm::PointerType::getUnqual(VTy));
    return Builder.CreateLoad(Ops[0]);
  case NEON::BI__builtin_neon_vst1_v:
  case NEON::BI__builtin_neon_vst1q_v:
    Ops[0] = Builder.CreateBitCast(Ops[0], llvm::PointerType::getUnqual(VTy));
    Ops[1] = Builder.CreateBitCast(Ops[1], VTy);
    return Builder.CreateStore(Ops[1], Ops[0]);
  case NEON::BI__builtin_neon_vld1_lane_v:
  case NEON::BI__builtin_neon_vld1q_lane_v:
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ty = llvm::PointerType::getUnqual(VTy->getElementType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[0] = Builder.CreateLoad(Ops[0]);
    return Builder.CreateInsertElement(Ops[1], Ops[0], Ops[2], "vld1_lane");
  case NEON::BI__builtin_neon_vld1_dup_v:
  case NEON::BI__builtin_neon_vld1q_dup_v: {
    Value *V = UndefValue::get(Ty);
    Ty = llvm::PointerType::getUnqual(VTy->getElementType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[0] = Builder.CreateLoad(Ops[0]);
    llvm::Constant *CI = ConstantInt::get(Int32Ty, 0);
    Ops[0] = Builder.CreateInsertElement(V, Ops[0], CI);
    return EmitNeonSplat(Ops[0], CI);
  }
  case NEON::BI__builtin_neon_vst1_lane_v:
  case NEON::BI__builtin_neon_vst1q_lane_v:
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[1] = Builder.CreateExtractElement(Ops[1], Ops[2]);
    Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
    return Builder.CreateStore(Ops[1], Builder.CreateBitCast(Ops[0], Ty));
  case NEON::BI__builtin_neon_vld2_v:
  case NEON::BI__builtin_neon_vld2q_v: {
    llvm::Type *PTy = llvm::PointerType::getUnqual(VTy);
    Ops[1] = Builder.CreateBitCast(Ops[1], PTy);
    llvm::Type *Tys[2] = { VTy, PTy };
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_ld2, Tys);
    Ops[1] = Builder.CreateCall(F, Ops[1], "vld2");
    Ops[0] = Builder.CreateBitCast(Ops[0],
                llvm::PointerType::getUnqual(Ops[1]->getType()));
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vld3_v:
  case NEON::BI__builtin_neon_vld3q_v: {
    llvm::Type *PTy = llvm::PointerType::getUnqual(VTy);
    Ops[1] = Builder.CreateBitCast(Ops[1], PTy);
    llvm::Type *Tys[2] = { VTy, PTy };
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_ld3, Tys);
    Ops[1] = Builder.CreateCall(F, Ops[1], "vld3");
    Ops[0] = Builder.CreateBitCast(Ops[0],
                llvm::PointerType::getUnqual(Ops[1]->getType()));
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vld4_v:
  case NEON::BI__builtin_neon_vld4q_v: {
    llvm::Type *PTy = llvm::PointerType::getUnqual(VTy);
    Ops[1] = Builder.CreateBitCast(Ops[1], PTy);
    llvm::Type *Tys[2] = { VTy, PTy };
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_ld4, Tys);
    Ops[1] = Builder.CreateCall(F, Ops[1], "vld4");
    Ops[0] = Builder.CreateBitCast(Ops[0],
                llvm::PointerType::getUnqual(Ops[1]->getType()));
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vld2_dup_v: {
    llvm::Type *PTy =
      llvm::PointerType::getUnqual(VTy->getElementType());
    Ops[1] = Builder.CreateBitCast(Ops[1], PTy);
    llvm::Type *Tys[2] = { VTy, PTy };
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_ld2r, Tys);
    Ops[1] = Builder.CreateCall(F, Ops[1], "vld2");
    Ops[0] = Builder.CreateBitCast(Ops[0],
                llvm::PointerType::getUnqual(Ops[1]->getType()));
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vld3_dup_v: {
    llvm::Type *PTy =
      llvm::PointerType::getUnqual(VTy->getElementType());
    Ops[1] = Builder.CreateBitCast(Ops[1], PTy);
    llvm::Type *Tys[2] = { VTy, PTy };
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_ld3r, Tys);
    Ops[1] = Builder.CreateCall(F, Ops[1], "vld3");
    Ops[0] = Builder.CreateBitCast(Ops[0],
                llvm::PointerType::getUnqual(Ops[1]->getType()));
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vld4_dup_v: {
    llvm::Type *PTy =
      llvm::PointerType::getUnqual(VTy->getElementType());
    Ops[1] = Builder.CreateBitCast(Ops[1], PTy);
    llvm::Type *Tys[2] = { VTy, PTy };
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_ld4r, Tys);
    Ops[1] = Builder.CreateCall(F, Ops[1], "vld4");
    Ops[0] = Builder.CreateBitCast(Ops[0],
                llvm::PointerType::getUnqual(Ops[1]->getType()));
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vld2_lane_v:
  case NEON::BI__builtin_neon_vld2q_lane_v: {
    llvm::Type *Tys[2] = { VTy, Ops[1]->getType() };
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_ld2lane, Tys);
    Ops.push_back(Ops[1]);
    Ops.erase(Ops.begin()+1);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);
    Ops[3] = Builder.CreateZExt(Ops[3],
                llvm::IntegerType::get(getLLVMContext(), 64));
    Ops[1] = Builder.CreateCall(F,
                ArrayRef<Value*>(Ops).slice(1), "vld2_lane");
    Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vld3_lane_v:
  case NEON::BI__builtin_neon_vld3q_lane_v: {
    llvm::Type *Tys[2] = { VTy, Ops[1]->getType() };
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_ld3lane, Tys);
    Ops.push_back(Ops[1]);
    Ops.erase(Ops.begin()+1);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);
    Ops[3] = Builder.CreateBitCast(Ops[3], Ty);
    Ops[4] = Builder.CreateZExt(Ops[4],
                llvm::IntegerType::get(getLLVMContext(), 64));
    Ops[1] = Builder.CreateCall(F,
                ArrayRef<Value*>(Ops).slice(1), "vld3_lane");
    Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vld4_lane_v:
  case NEON::BI__builtin_neon_vld4q_lane_v: {
    llvm::Type *Tys[2] = { VTy, Ops[1]->getType() };
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_ld4lane, Tys);
    Ops.push_back(Ops[1]);
    Ops.erase(Ops.begin()+1);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);
    Ops[3] = Builder.CreateBitCast(Ops[3], Ty);
    Ops[4] = Builder.CreateBitCast(Ops[4], Ty);
    Ops[5] = Builder.CreateZExt(Ops[5],
                llvm::IntegerType::get(getLLVMContext(), 64));
    Ops[1] = Builder.CreateCall(F,
                ArrayRef<Value*>(Ops).slice(1), "vld4_lane");
    Ty = llvm::PointerType::getUnqual(Ops[1]->getType());
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case NEON::BI__builtin_neon_vst2_v:
  case NEON::BI__builtin_neon_vst2q_v: {
    Ops.push_back(Ops[0]);
    Ops.erase(Ops.begin());
    llvm::Type *Tys[2] = { VTy, Ops[2]->getType() };
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_st2, Tys),
                        Ops, "");
  }
  case NEON::BI__builtin_neon_vst2_lane_v:
  case NEON::BI__builtin_neon_vst2q_lane_v: {
    Ops.push_back(Ops[0]);
    Ops.erase(Ops.begin());
    Ops[2] = Builder.CreateZExt(Ops[2],
                llvm::IntegerType::get(getLLVMContext(), 64));
    llvm::Type *Tys[2] = { VTy, Ops[3]->getType() };
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_st2lane, Tys),
                        Ops, "");
  }
  case NEON::BI__builtin_neon_vst3_v:
  case NEON::BI__builtin_neon_vst3q_v: {
    Ops.push_back(Ops[0]);
    Ops.erase(Ops.begin());
    llvm::Type *Tys[2] = { VTy, Ops[3]->getType() };
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_st3, Tys),
                        Ops, "");
  }
  case NEON::BI__builtin_neon_vst3_lane_v:
  case NEON::BI__builtin_neon_vst3q_lane_v: {
    Ops.push_back(Ops[0]);
    Ops.erase(Ops.begin());
    Ops[3] = Builder.CreateZExt(Ops[3],
                llvm::IntegerType::get(getLLVMContext(), 64));
    llvm::Type *Tys[2] = { VTy, Ops[4]->getType() };
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_st3lane, Tys),
                        Ops, "");
  }
  case NEON::BI__builtin_neon_vst4_v:
  case NEON::BI__builtin_neon_vst4q_v: {
    Ops.push_back(Ops[0]);
    Ops.erase(Ops.begin());
    llvm::Type *Tys[2] = { VTy, Ops[4]->getType() };
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_st4, Tys),
                        Ops, "");
  }
  case NEON::BI__builtin_neon_vst4_lane_v:
  case NEON::BI__builtin_neon_vst4q_lane_v: {
    Ops.push_back(Ops[0]);
    Ops.erase(Ops.begin());
    Ops[4] = Builder.CreateZExt(Ops[4],
                llvm::IntegerType::get(getLLVMContext(), 64));
    llvm::Type *Tys[2] = { VTy, Ops[5]->getType() };
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_st4lane, Tys),
                        Ops, "");
  }
  case NEON::BI__builtin_neon_vqmovn_v:
    Int = usgn ? Intrinsic::arm64_neon_uqxtn : Intrinsic::arm64_neon_sqxtn;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty), Ops, "vqmovn");
  case NEON::BI__builtin_neon_vqmovun_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_sqxtun, Ty),
                        Ops, "vqmovun");
  case NEON::BI__builtin_neon_vabs_v:
  case NEON::BI__builtin_neon_vabsq_v:
      return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_abs, Ty),
                          Ops, "vabs");
  case NEON::BI__builtin_neon_vqabs_v:
  case NEON::BI__builtin_neon_vqabsq_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_sqabs, Ty),
                        Ops, "vqabs");
  case NEON::BI__builtin_neon_vqneg_v:
  case NEON::BI__builtin_neon_vqnegq_v:
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_sqneg, Ty),
                        Ops, "vqneg");
  case NEON::BI__builtin_neon_vclz_v:
  case NEON::BI__builtin_neon_vclzq_v: {
    // generate target-independent intrinsic; also need to add second argument
    // for whether or not clz of zero is undefined; on ARM64 it isn't.
    Function *F = CGM.getIntrinsic(Intrinsic::ctlz, Ty);
    Ops.push_back(Builder.getInt1(Target.isCLZForZeroUndef()));
    return EmitNeonCall(F, Ops, "vclz");
  }
  case NEON::BI__builtin_neon_vcls_v:
  case NEON::BI__builtin_neon_vclsq_v: {
    Function *F = CGM.getIntrinsic(Intrinsic::arm64_neon_cls, Ty);
    return EmitNeonCall(F, Ops, "vcls");
  }
  case NEON::BI__builtin_neon_vcnt_v:
  case NEON::BI__builtin_neon_vcntq_v: {
    Function *F = CGM.getIntrinsic(Intrinsic::ctpop, Ty);
    return EmitNeonCall(F, Ops, "vcnt");
  }
  case NEON::BI__builtin_neon_vrecpe_v:
  case NEON::BI__builtin_neon_vrecpeq_v:
    Int = Intrinsic::arm64_neon_urecpe;
    if (Ty->isFPOrFPVectorTy()) Int = Intrinsic::arm64_neon_frecpe;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty),
                        Ops, "vrecpe");
  case NEON::BI__builtin_neon_vrsqrte_v:
  case NEON::BI__builtin_neon_vrsqrteq_v:
    Int = Intrinsic::arm64_neon_ursqrte;
    if (Ty->isFPOrFPVectorTy()) Int = Intrinsic::arm64_neon_frsqrte;
    return EmitNeonCall(CGM.getIntrinsic(Int, Ty),
                                         Ops, "vrsqrte");
  case NEON::BI__builtin_neon_vtrn_v:
  case NEON::BI__builtin_neon_vtrnq_v: {
    Ops[0] = Builder.CreateBitCast(Ops[0], llvm::PointerType::getUnqual(Ty));
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);
    Value *SV = 0;

    for (unsigned vi = 0; vi != 2; ++vi) {
      SmallVector<Constant*, 16> Indices;
      for (unsigned i = 0, e = VTy->getNumElements(); i != e; i += 2) {
        Indices.push_back(ConstantInt::get(Int32Ty, i+vi));
        Indices.push_back(ConstantInt::get(Int32Ty, i+e+vi));
      }
      Value *Addr = Builder.CreateConstInBoundsGEP1_32(Ops[0], vi);
      SV = llvm::ConstantVector::get(Indices);
      SV = Builder.CreateShuffleVector(Ops[1], Ops[2], SV, "vtrn");
      SV = Builder.CreateStore(SV, Addr);
    }
    return SV;
  }
  case NEON::BI__builtin_neon_vuzp_v:
  case NEON::BI__builtin_neon_vuzpq_v: {
    Ops[0] = Builder.CreateBitCast(Ops[0], llvm::PointerType::getUnqual(Ty));
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);
    Value *SV = 0;

    for (unsigned vi = 0; vi != 2; ++vi) {
      SmallVector<Constant*, 16> Indices;
      for (unsigned i = 0, e = VTy->getNumElements(); i != e; ++i)
        Indices.push_back(ConstantInt::get(Int32Ty, 2*i+vi));

      Value *Addr = Builder.CreateConstInBoundsGEP1_32(Ops[0], vi);
      SV = llvm::ConstantVector::get(Indices);
      SV = Builder.CreateShuffleVector(Ops[1], Ops[2], SV, "vuzp");
      SV = Builder.CreateStore(SV, Addr);
    }
    return SV;
  }
  case NEON::BI__builtin_neon_vzip_v: 
  case NEON::BI__builtin_neon_vzipq_v: {
    Ops[0] = Builder.CreateBitCast(Ops[0], llvm::PointerType::getUnqual(Ty));
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    Ops[2] = Builder.CreateBitCast(Ops[2], Ty);
    Value *SV = 0;

    for (unsigned vi = 0; vi != 2; ++vi) {
      SmallVector<Constant*, 16> Indices;
      for (unsigned i = 0, e = VTy->getNumElements(); i != e; i += 2) {
        Indices.push_back(ConstantInt::get(Int32Ty, (i + vi*e) >> 1));
        Indices.push_back(ConstantInt::get(Int32Ty, ((i + vi*e) >> 1)+e));
      }
      Value *Addr = Builder.CreateConstInBoundsGEP1_32(Ops[0], vi);
      SV = llvm::ConstantVector::get(Indices);
      SV = Builder.CreateShuffleVector(Ops[1], Ops[2], SV, "vzip");
      SV = Builder.CreateStore(SV, Addr);
    }
    return SV;
  }
  case NEON::BI__builtin_neon_vtbl1q_v:
  case NEON::BI__builtin_neon_vqtbl1q_v: {
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_tbl1, Ty),
                        Ops, "vtbl1");
  }
  case NEON::BI__builtin_neon_vtbl2q_v:
  case NEON::BI__builtin_neon_vqtbl2q_v: {
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_tbl2, Ty),
                        Ops, "vtbl2");
  }
  case NEON::BI__builtin_neon_vtbl3q_v:
  case NEON::BI__builtin_neon_vqtbl3q_v: {
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_tbl3, Ty),
                        Ops, "vtbl3");
  }
  case NEON::BI__builtin_neon_vtbl4q_v:
  case NEON::BI__builtin_neon_vqtbl4q_v: {
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_tbl4, Ty),
                        Ops, "vtbl4");
  }
  case NEON::BI__builtin_neon_vtbx1q_v:
  case NEON::BI__builtin_neon_vqtbx1q_v: {
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_tbx1, Ty),
                        Ops, "vtbx1");
  }
  case NEON::BI__builtin_neon_vtbx2q_v:
  case NEON::BI__builtin_neon_vqtbx2q_v: {
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_tbx2, Ty),
                        Ops, "vtbx2");
  }
  case NEON::BI__builtin_neon_vtbx3q_v:
  case NEON::BI__builtin_neon_vqtbx3q_v: {
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_tbx3, Ty),
                        Ops, "vtbx3");
  }
  case NEON::BI__builtin_neon_vtbx4q_v:
  case NEON::BI__builtin_neon_vqtbx4q_v: {
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_tbx4, Ty),
                        Ops, "vtbx4");
  }
  case NEON::BI__builtin_neon_vabdl_high_v: {
    Int = usgn ? Intrinsic::arm64_neon_uabdl : Intrinsic::arm64_neon_sabdl;
    unsigned NumElts = VTy->getNumElements();
    unsigned BitWidth =cast<IntegerType>(VTy->getElementType())->getBitWidth();
    llvm::Type *DInt =
      llvm::IntegerType::get(getLLVMContext(), BitWidth/2);
    llvm::Type *ArgTy = llvm::VectorType::get(DInt, NumElts);
    Ops[0] = EmitExtractHigh(Ops[0], ArgTy);
    Ops[1] = EmitExtractHigh(Ops[1], ArgTy);
    return EmitNeonCall(CGM.getIntrinsic(Int, VTy), Ops, "vabdl_high");
  }
  case NEON::BI__builtin_neon_vabal_high_v: {
    Int = usgn ? Intrinsic::arm64_neon_uabdl : Intrinsic::arm64_neon_sabdl;
    unsigned NumElts = VTy->getNumElements();
    unsigned BitWidth =cast<IntegerType>(VTy->getElementType())->getBitWidth();
    llvm::Type *DInt =
      llvm::IntegerType::get(getLLVMContext(), BitWidth/2);
    llvm::Type *ArgTy = llvm::VectorType::get(DInt, NumElts);
    Ops[1] = EmitExtractHigh(Ops[1], ArgTy);
    Ops[2] = EmitExtractHigh(Ops[2], ArgTy);

    SmallVector<llvm::Value*, 2> TmpOps(Ops.begin()+1, Ops.end());
    Ops[1] = EmitNeonCall(CGM.getIntrinsic(Int, VTy), TmpOps, "vabdl_high");
    return Builder.CreateAdd(Builder.CreateBitCast(Ops[0], Ty), Ops[1]);
  }
  case NEON::BI__builtin_neon_vaddw_high_v: {
    unsigned NumElts = VTy->getNumElements();
    unsigned BitWidth =cast<IntegerType>(VTy->getElementType())->getBitWidth();
    llvm::Type *DInt =
      llvm::IntegerType::get(getLLVMContext(), BitWidth/2);
    llvm::Type *ArgTy = llvm::VectorType::get(DInt, NumElts);
    Ops[1] = EmitExtractHigh(Ops[1], ArgTy);
    Ops[1] = usgn ? Builder.CreateZExt(Ops[1], VTy) :
                    Builder.CreateSExt(Ops[1], VTy);
    return Builder.CreateAdd(Builder.CreateBitCast(Ops[0], VTy), Ops[1]);
  }
  case NEON::BI__builtin_neon_vqdmull_high_v: {
    Int = Intrinsic::arm64_neon_sqdmull;
    unsigned NumElts = VTy->getNumElements();
    unsigned BitWidth =cast<IntegerType>(VTy->getElementType())->getBitWidth();
    llvm::Type *DInt =
      llvm::IntegerType::get(getLLVMContext(), BitWidth/2);
    llvm::Type *ArgTy = llvm::VectorType::get(DInt, NumElts);
    Ops[0] = EmitExtractHigh(Ops[0], ArgTy);
    Ops[1] = EmitExtractHigh(Ops[1], ArgTy);
    return EmitNeonCall(CGM.getIntrinsic(Int, VTy), Ops, "vqdmull_high");
  }
  case NEON::BI__builtin_neon_vqdmlal_high_v: {
    Int = Intrinsic::arm64_neon_sqdmull;
    unsigned NumElts = VTy->getNumElements();
    unsigned BitWidth =cast<IntegerType>(VTy->getElementType())->getBitWidth();
    llvm::Type *DInt =
      llvm::IntegerType::get(getLLVMContext(), BitWidth/2);
    llvm::Type *ArgTy = llvm::VectorType::get(DInt, NumElts);
    // Now we're ready to generate code: extract the elements...
    Ops[1] = EmitExtractHigh(Ops[1], ArgTy);
    Ops[2] = EmitExtractHigh(Ops[2], ArgTy);
    // ... perform an saturating doubling multiply ...
    SmallVector<llvm::Value*, 2> TmpOps(Ops.begin()+1, Ops.end());
    Ops[1] = EmitNeonCall(CGM.getIntrinsic(Int, VTy), TmpOps, "vqdmull_high");
    // ... and a saturating accumulate op.
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops.pop_back();
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_sqadd, VTy),
                        Ops, "vqadd");
  }
  case NEON::BI__builtin_neon_vqdmlsl_high_v: {
    Int = Intrinsic::arm64_neon_sqdmull;
    unsigned NumElts = VTy->getNumElements();
    unsigned BitWidth =cast<IntegerType>(VTy->getElementType())->getBitWidth();
    llvm::Type *DInt =
      llvm::IntegerType::get(getLLVMContext(), BitWidth/2);
    llvm::Type *ArgTy = llvm::VectorType::get(DInt, NumElts);
    // Now we're ready to generate code: extract the elements...
    Ops[1] = EmitExtractHigh(Ops[1], ArgTy);
    Ops[2] = EmitExtractHigh(Ops[2], ArgTy);
    // ... perform an saturating doubling multiply ...
    SmallVector<llvm::Value*, 2> TmpOps(Ops.begin()+1, Ops.end());
    Ops[1] = EmitNeonCall(CGM.getIntrinsic(Int, VTy), TmpOps, "vqdmull_high");
    // ... and a saturating accumulate op.
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops.pop_back();
    return EmitNeonCall(CGM.getIntrinsic(Intrinsic::arm64_neon_sqsub, VTy),
                        Ops, "vqsub");
  }
  case NEON::BI__builtin_neon_vsubl_high_v: {
    unsigned NumElts = VTy->getNumElements();
    unsigned BitWidth =cast<IntegerType>(VTy->getElementType())->getBitWidth();
    llvm::Type *DInt =
      llvm::IntegerType::get(getLLVMContext(), BitWidth/2);
    llvm::Type *ArgTy = llvm::VectorType::get(DInt, NumElts);
    Ops[0] = EmitExtractHigh(Ops[0], ArgTy);
    Ops[1] = EmitExtractHigh(Ops[1], ArgTy);
    if (usgn) {
      Ops[0] = Builder.CreateZExt(Ops[0], VTy);
      Ops[1] = Builder.CreateZExt(Ops[1], VTy);
    } else {
      Ops[0] = Builder.CreateSExt(Ops[0], VTy);
      Ops[1] = Builder.CreateSExt(Ops[1], VTy);
    }
    return Builder.CreateSub(Ops[0], Ops[1]);
  }
  case NEON::BI__builtin_neon_vsubw_high_v: {
    unsigned NumElts = VTy->getNumElements();
    unsigned BitWidth =cast<IntegerType>(VTy->getElementType())->getBitWidth();
    llvm::Type *DInt =
      llvm::IntegerType::get(getLLVMContext(), BitWidth/2);
    llvm::Type *ArgTy = llvm::VectorType::get(DInt, NumElts);
    Ops[1] = EmitExtractHigh(Ops[1], ArgTy);
    Ops[1] = usgn ? Builder.CreateZExt(Ops[1], VTy) :
                   Builder.CreateSExt(Ops[1], VTy);
    return Builder.CreateSub(Builder.CreateBitCast(Ops[0], VTy), Ops[1]);
  }
  case NEON::BI__builtin_neon_vqmovn_high_v:
  case NEON::BI__builtin_neon_vqmovun_high_v: {
    const char *Name;
    if (BuiltinID == NEON::BI__builtin_neon_vqmovn_high_v) {
      Int = usgn ? Intrinsic::arm64_neon_uqxtn : Intrinsic::arm64_neon_sqxtn;
      Name = "vqmovn_high";
    } else {
      Int = Intrinsic::arm64_neon_sqxtun;
      Name = "vqmovun_high";
    }
    unsigned NumElts = VTy->getNumElements();
    llvm::Type *ArgTy = llvm::VectorType::get(VTy->getElementType(), NumElts/2);
    SmallVector<Value *, 1> CallOps;
    CallOps.push_back(Ops[1]);
    Value *Shifted = EmitNeonCall(CGM.getIntrinsic(Int, ArgTy), CallOps, Name);

    return EmitConcatVectors(Ops[0], Shifted, ArgTy);
  }
  case NEON::BI__builtin_neon_vrshrn_high_n_v:
  case NEON::BI__builtin_neon_vshrn_high_n_v:
  case NEON::BI__builtin_neon_vqshrun_high_n_v:
  case NEON::BI__builtin_neon_vqrshrun_high_n_v:
  case NEON::BI__builtin_neon_vqshrn_high_n_v:
  case NEON::BI__builtin_neon_vqrshrn_high_n_v: {
    const char *Name;
    switch (BuiltinID) {
    default:
      assert(0 && "Unexpected BuiltinID!");
    case NEON::BI__builtin_neon_vshrn_high_n_v: {
      unsigned NumElts = VTy->getNumElements();
      llvm::VectorType *ConcatTy =
          llvm::VectorType::get(VTy->getElementType(), NumElts / 2);
      llvm::VectorType *ArgTy =
          llvm::VectorType::getExtendedElementVectorType(ConcatTy);

      Ops[1] = Builder.CreateBitCast(Ops[1], ArgTy);
      Ops[2] = EmitNeonShiftVector(Ops[2], ArgTy, false);
      llvm::Value *Shift = Builder.CreateLShr(Ops[1], Ops[2]);
      llvm::Value *Trunc = Builder.CreateTrunc(Shift, ConcatTy);
      return EmitConcatVectors(Ops[0], Trunc, ConcatTy);
      break;
    }
    case NEON::BI__builtin_neon_vrshrn_high_n_v:
      Int = Intrinsic::arm64_neon_rshrn;
      Name = "vrshrn_high_n";
      break;
    case NEON::BI__builtin_neon_vqshrun_high_n_v:
      Int = Intrinsic::arm64_neon_sqshrun;
      Name = "vqshrun_high_n";
      break;
    case NEON::BI__builtin_neon_vqrshrun_high_n_v:
      Int = Intrinsic::arm64_neon_sqrshrun;
      Name = "vqrshrun_high_n";
      break;
    case NEON::BI__builtin_neon_vqshrn_high_n_v:
      Int = usgn ? Intrinsic::arm64_neon_uqshrn :
        Intrinsic::arm64_neon_sqshrn;
      Name = "vqshrun_high_n";
      break;
    case NEON::BI__builtin_neon_vqrshrn_high_n_v:
      Int = usgn ? Intrinsic::arm64_neon_uqrshrn :
        Intrinsic::arm64_neon_sqrshrn;
      Name = "vqshrun_high_n";
      break;
    }
    unsigned NumElts = VTy->getNumElements();
    llvm::Type *ArgTy = llvm::VectorType::get(VTy->getElementType(), NumElts/2);
    SmallVector<Value *, 2> CallOps;
    CallOps.push_back(Ops[1]);
    CallOps.push_back(Ops[2]);
    Value *Shifted = EmitNeonCall(CGM.getIntrinsic(Int, ArgTy), CallOps, Name);

    return EmitConcatVectors(Ops[0], Shifted, ArgTy);
  }
  case NEON::BI__builtin_neon_vshll_high_n_v: {
    llvm::Type *SrcTy = llvm::VectorType::getTruncatedElementVectorType(VTy);
    Ops[0] = EmitExtractHigh(Ops[0], SrcTy);
    Ops[0] = Builder.CreateBitCast(Ops[0], SrcTy);
    if (usgn)
      Ops[0] = Builder.CreateZExt(Ops[0], VTy);
    else
      Ops[0] = Builder.CreateSExt(Ops[0], VTy);
    Ops[1] = EmitNeonShiftVector(Ops[1], VTy, false);
    return Builder.CreateShl(Ops[0], Ops[1], "vshll_n");
  }
  case NEON::BI__builtin_neon_vrsubhn_high_v:
  case NEON::BI__builtin_neon_vsubhn_high_v:
  case NEON::BI__builtin_neon_vraddhn_high_v:
  case NEON::BI__builtin_neon_vaddhn_high_v: {
    const char *Name;
    switch (BuiltinID) {
    case NEON::BI__builtin_neon_vrsubhn_high_v:
      Int = Intrinsic::arm64_neon_rsubhn;
      Name = "vrsubhn_high";
      break;
    case NEON::BI__builtin_neon_vsubhn_high_v:
      Int = Intrinsic::arm64_neon_subhn;
      Name = "vsubhn_high";
      break;
    case NEON::BI__builtin_neon_vraddhn_high_v:
      Int = Intrinsic::arm64_neon_raddhn;
      Name = "vraddhn_high";
      break;
    case NEON::BI__builtin_neon_vaddhn_high_v:
      Int = Intrinsic::arm64_neon_addhn;
      Name = "vaddhn_high";
      break;
    }
    unsigned NumElts = VTy->getNumElements();
    llvm::Type *ArgTy = llvm::VectorType::get(VTy->getElementType(), NumElts/2);
    SmallVector<Value *, 2> CallOps;
    CallOps.push_back(Ops[1]);
    CallOps.push_back(Ops[2]);
    Value *Shifted = EmitNeonCall(CGM.getIntrinsic(Int, ArgTy), CallOps, Name);

    return EmitConcatVectors(Ops[0], Shifted, ArgTy);
  }
  case NEON::BI__builtin_neon_vmovn_high_v: {
    unsigned NumElts = VTy->getNumElements();
    llvm::VectorType *NarrowTy = llvm::VectorType::get(VTy->getElementType(),
                                                       NumElts/2);
    llvm::Type *NarrowQTy =
        llvm::VectorType::getExtendedElementVectorType(NarrowTy);
    Ops[1] = Builder.CreateBitCast(Ops[1], NarrowQTy);
    Ops[1] = Builder.CreateTrunc(Ops[1], NarrowTy, "vmovn_high");
    return EmitConcatVectors(Ops[0], Ops[1], NarrowTy);
  }
  case NEON::BI__builtin_neon_vtrn1_v:
  case NEON::BI__builtin_neon_vtrn1q_v: {
    Ops[0] = Builder.CreateBitCast(Ops[0], VTy);
    Ops[1] = Builder.CreateBitCast(Ops[1], VTy);
    SmallVector<Constant*, 16> Indices;
    for (unsigned i = 0, e = VTy->getNumElements(); i != e; i += 2) {
      Indices.push_back(ConstantInt::get(Int32Ty, i));
      Indices.push_back(ConstantInt::get(Int32Ty, i+e));
    }
    Ops.push_back(llvm::ConstantVector::get(Indices));
    return Builder.CreateShuffleVector(Ops[0], Ops[1], Ops[2], "vtrn1");
  }
  case NEON::BI__builtin_neon_vtrn2_v:
  case NEON::BI__builtin_neon_vtrn2q_v: {
    Ops[0] = Builder.CreateBitCast(Ops[0], VTy);
    Ops[1] = Builder.CreateBitCast(Ops[1], VTy);
    SmallVector<Constant*, 16> Indices;
    for (unsigned i = 0, e = VTy->getNumElements(); i != e; i += 2) {
      Indices.push_back(ConstantInt::get(Int32Ty, i+1));
      Indices.push_back(ConstantInt::get(Int32Ty, i+e+1));
    }
    Ops.push_back(llvm::ConstantVector::get(Indices));
    return Builder.CreateShuffleVector(Ops[0], Ops[1], Ops[2], "vtrn2");
  }
  case NEON::BI__builtin_neon_vuzp1_v:
  case NEON::BI__builtin_neon_vuzp1q_v: {
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    SmallVector<Constant*, 16> Indices;
    for (unsigned i = 0, e = VTy->getNumElements(); i != e; ++i)
      Indices.push_back(ConstantInt::get(Int32Ty, 2*i));
    Ops.push_back(llvm::ConstantVector::get(Indices));
    return Builder.CreateShuffleVector(Ops[0], Ops[1], Ops[2], "vuzp1");
  }
  case NEON::BI__builtin_neon_vuzp2_v:
  case NEON::BI__builtin_neon_vuzp2q_v: {
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    SmallVector<Constant*, 16> Indices;
    for (unsigned i = 0, e = VTy->getNumElements(); i != e; ++i)
      Indices.push_back(ConstantInt::get(Int32Ty, 2*i+1));
    Ops.push_back(llvm::ConstantVector::get(Indices));
    return Builder.CreateShuffleVector(Ops[0], Ops[1], Ops[2], "vuzp2");
  }
  case NEON::BI__builtin_neon_vzip1_v:
  case NEON::BI__builtin_neon_vzip1q_v: {
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    SmallVector<Constant*, 16> Indices;
    for (unsigned i = 0, e = VTy->getNumElements(); i != e; i += 2) {
      Indices.push_back(ConstantInt::get(Int32Ty, i >> 1));
      Indices.push_back(ConstantInt::get(Int32Ty, (i >> 1)+e));
    }
    Ops.push_back(llvm::ConstantVector::get(Indices));
    return Builder.CreateShuffleVector(Ops[0], Ops[1], Ops[2], "vzip1");
  }
  case NEON::BI__builtin_neon_vzip2_v:
  case NEON::BI__builtin_neon_vzip2q_v: {
    Ops[0] = Builder.CreateBitCast(Ops[0], Ty);
    Ops[1] = Builder.CreateBitCast(Ops[1], Ty);
    SmallVector<Constant*, 16> Indices;
    for (unsigned i = 0, e = VTy->getNumElements(); i != e; i += 2) {
      Indices.push_back(ConstantInt::get(Int32Ty, (i+e) >> 1));
      Indices.push_back(ConstantInt::get(Int32Ty, ((i+e) >> 1)+e));
    }
    Ops.push_back(llvm::ConstantVector::get(Indices));
    return Builder.CreateShuffleVector(Ops[0], Ops[1], Ops[2], "vzip2");
  }
  }
}

llvm::Value *CodeGenFunction::
BuildVector(ArrayRef<llvm::Value*> Ops) {
  assert((Ops.size() & (Ops.size() - 1)) == 0 &&
         "Not a power-of-two sized vector!");
  bool AllConstants = true;
  for (unsigned i = 0, e = Ops.size(); i != e && AllConstants; ++i)
    AllConstants &= isa<Constant>(Ops[i]);

  // If this is a constant vector, create a ConstantVector.
  if (AllConstants) {
    SmallVector<llvm::Constant*, 16> CstOps;
    for (unsigned i = 0, e = Ops.size(); i != e; ++i)
      CstOps.push_back(cast<Constant>(Ops[i]));
    return llvm::ConstantVector::get(CstOps);
  }

  // Otherwise, insertelement the values to build the vector.
  Value *Result =
    llvm::UndefValue::get(llvm::VectorType::get(Ops[0]->getType(), Ops.size()));

  for (unsigned i = 0, e = Ops.size(); i != e; ++i)
    Result = Builder.CreateInsertElement(Result, Ops[i], Builder.getInt32(i));

  return Result;
}

Value *CodeGenFunction::EmitX86BuiltinExpr(unsigned BuiltinID,
                                           const CallExpr *E) {
  SmallVector<Value*, 4> Ops;

  // Find out if any arguments are required to be integer constant expressions.
  unsigned ICEArguments = 0;
  ASTContext::GetBuiltinTypeError Error;
  getContext().GetBuiltinType(BuiltinID, Error, &ICEArguments);
  assert(Error == ASTContext::GE_None && "Should not codegen an error");

  for (unsigned i = 0, e = E->getNumArgs(); i != e; i++) {
    // If this is a normal argument, just emit it as a scalar.
    if ((ICEArguments & (1 << i)) == 0) {
      Ops.push_back(EmitScalarExpr(E->getArg(i)));
      continue;
    }

    // If this is required to be a constant, constant fold it so that we know
    // that the generated intrinsic gets a ConstantInt.
    llvm::APSInt Result;
    bool IsConst = E->getArg(i)->isIntegerConstantExpr(Result, getContext());
    assert(IsConst && "Constant arg isn't actually constant?"); (void)IsConst;
    Ops.push_back(llvm::ConstantInt::get(getLLVMContext(), Result));
  }

  switch (BuiltinID) {
  default: return 0;
  case X86::BI__builtin_ia32_vec_init_v8qi:
  case X86::BI__builtin_ia32_vec_init_v4hi:
  case X86::BI__builtin_ia32_vec_init_v2si:
    return Builder.CreateBitCast(BuildVector(Ops),
                                 llvm::Type::getX86_MMXTy(getLLVMContext()));
  case X86::BI__builtin_ia32_vec_ext_v2si:
    return Builder.CreateExtractElement(Ops[0],
                                  llvm::ConstantInt::get(Ops[1]->getType(), 0));
  case X86::BI__builtin_ia32_ldmxcsr: {
    Value *Tmp = CreateMemTemp(E->getArg(0)->getType());
    Builder.CreateStore(Ops[0], Tmp);
    return Builder.CreateCall(CGM.getIntrinsic(Intrinsic::x86_sse_ldmxcsr),
                              Builder.CreateBitCast(Tmp, Int8PtrTy));
  }
  case X86::BI__builtin_ia32_stmxcsr: {
    Value *Tmp = CreateMemTemp(E->getType());
    Builder.CreateCall(CGM.getIntrinsic(Intrinsic::x86_sse_stmxcsr),
                       Builder.CreateBitCast(Tmp, Int8PtrTy));
    return Builder.CreateLoad(Tmp, "stmxcsr");
  }
  case X86::BI__builtin_ia32_storehps:
  case X86::BI__builtin_ia32_storelps: {
    llvm::Type *PtrTy = llvm::PointerType::getUnqual(Int64Ty);
    llvm::Type *VecTy = llvm::VectorType::get(Int64Ty, 2);

    // cast val v2i64
    Ops[1] = Builder.CreateBitCast(Ops[1], VecTy, "cast");

    // extract (0, 1)
    unsigned Index = BuiltinID == X86::BI__builtin_ia32_storelps ? 0 : 1;
    llvm::Value *Idx = llvm::ConstantInt::get(Int32Ty, Index);
    Ops[1] = Builder.CreateExtractElement(Ops[1], Idx, "extract");

    // cast pointer to i64 & store
    Ops[0] = Builder.CreateBitCast(Ops[0], PtrTy);
    return Builder.CreateStore(Ops[1], Ops[0]);
  }
  case X86::BI__builtin_ia32_palignr: {
    unsigned shiftVal = cast<llvm::ConstantInt>(Ops[2])->getZExtValue();

    // If palignr is shifting the pair of input vectors less than 9 bytes,
    // emit a shuffle instruction.
    if (shiftVal <= 8) {
      SmallVector<llvm::Constant*, 8> Indices;
      for (unsigned i = 0; i != 8; ++i)
        Indices.push_back(llvm::ConstantInt::get(Int32Ty, shiftVal + i));

      Value* SV = llvm::ConstantVector::get(Indices);
      return Builder.CreateShuffleVector(Ops[1], Ops[0], SV, "palignr");
    }

    // If palignr is shifting the pair of input vectors more than 8 but less
    // than 16 bytes, emit a logical right shift of the destination.
    if (shiftVal < 16) {
      // MMX has these as 1 x i64 vectors for some odd optimization reasons.
      llvm::Type *VecTy = llvm::VectorType::get(Int64Ty, 1);

      Ops[0] = Builder.CreateBitCast(Ops[0], VecTy, "cast");
      Ops[1] = llvm::ConstantInt::get(VecTy, (shiftVal-8) * 8);

      // create i32 constant
      llvm::Function *F = CGM.getIntrinsic(Intrinsic::x86_mmx_psrl_q);
      return Builder.CreateCall(F, makeArrayRef(&Ops[0], 2), "palignr");
    }

    // If palignr is shifting the pair of vectors more than 16 bytes, emit zero.
    return llvm::Constant::getNullValue(ConvertType(E->getType()));
  }
  case X86::BI__builtin_ia32_palignr128: {
    unsigned shiftVal = cast<llvm::ConstantInt>(Ops[2])->getZExtValue();

    // If palignr is shifting the pair of input vectors less than 17 bytes,
    // emit a shuffle instruction.
    if (shiftVal <= 16) {
      SmallVector<llvm::Constant*, 16> Indices;
      for (unsigned i = 0; i != 16; ++i)
        Indices.push_back(llvm::ConstantInt::get(Int32Ty, shiftVal + i));

      Value* SV = llvm::ConstantVector::get(Indices);
      return Builder.CreateShuffleVector(Ops[1], Ops[0], SV, "palignr");
    }

    // If palignr is shifting the pair of input vectors more than 16 but less
    // than 32 bytes, emit a logical right shift of the destination.
    if (shiftVal < 32) {
      llvm::Type *VecTy = llvm::VectorType::get(Int64Ty, 2);

      Ops[0] = Builder.CreateBitCast(Ops[0], VecTy, "cast");
      Ops[1] = llvm::ConstantInt::get(Int32Ty, (shiftVal-16) * 8);

      // create i32 constant
      llvm::Function *F = CGM.getIntrinsic(Intrinsic::x86_sse2_psrl_dq);
      return Builder.CreateCall(F, makeArrayRef(&Ops[0], 2), "palignr");
    }

    // If palignr is shifting the pair of vectors more than 32 bytes, emit zero.
    return llvm::Constant::getNullValue(ConvertType(E->getType()));
  }
  case X86::BI__builtin_ia32_palignr256: {
    unsigned shiftVal = cast<llvm::ConstantInt>(Ops[2])->getZExtValue();

    // If palignr is shifting the pair of input vectors less than 17 bytes,
    // emit a shuffle instruction.
    if (shiftVal <= 16) {
      SmallVector<llvm::Constant*, 32> Indices;
      // 256-bit palignr operates on 128-bit lanes so we need to handle that
      for (unsigned l = 0; l != 2; ++l) {
        unsigned LaneStart = l * 16;
        unsigned LaneEnd = (l+1) * 16;
        for (unsigned i = 0; i != 16; ++i) {
          unsigned Idx = shiftVal + i + LaneStart;
          if (Idx >= LaneEnd) Idx += 16; // end of lane, switch operand
          Indices.push_back(llvm::ConstantInt::get(Int32Ty, Idx));
        }
      }

      Value* SV = llvm::ConstantVector::get(Indices);
      return Builder.CreateShuffleVector(Ops[1], Ops[0], SV, "palignr");
    }

    // If palignr is shifting the pair of input vectors more than 16 but less
    // than 32 bytes, emit a logical right shift of the destination.
    if (shiftVal < 32) {
      llvm::Type *VecTy = llvm::VectorType::get(Int64Ty, 4);

      Ops[0] = Builder.CreateBitCast(Ops[0], VecTy, "cast");
      Ops[1] = llvm::ConstantInt::get(Int32Ty, (shiftVal-16) * 8);

      // create i32 constant
      llvm::Function *F = CGM.getIntrinsic(Intrinsic::x86_avx2_psrl_dq);
      return Builder.CreateCall(F, makeArrayRef(&Ops[0], 2), "palignr");
    }

    // If palignr is shifting the pair of vectors more than 32 bytes, emit zero.
    return llvm::Constant::getNullValue(ConvertType(E->getType()));
  }
  case X86::BI__builtin_ia32_movntps:
  case X86::BI__builtin_ia32_movntps256:
  case X86::BI__builtin_ia32_movntpd:
  case X86::BI__builtin_ia32_movntpd256:
  case X86::BI__builtin_ia32_movntdq:
  case X86::BI__builtin_ia32_movntdq256:
  case X86::BI__builtin_ia32_movnti:
  case X86::BI__builtin_ia32_movnti64: {
    llvm::MDNode *Node = llvm::MDNode::get(getLLVMContext(),
                                           Builder.getInt32(1));

    // Convert the type of the pointer to a pointer to the stored type.
    Value *BC = Builder.CreateBitCast(Ops[0],
                                llvm::PointerType::getUnqual(Ops[1]->getType()),
                                      "cast");
    StoreInst *SI = Builder.CreateStore(Ops[1], BC);
    SI->setMetadata(CGM.getModule().getMDKindID("nontemporal"), Node);

    // If the operand is an integer, we can't assume alignment. Otherwise,
    // assume natural alignment.
    QualType ArgTy = E->getArg(1)->getType();
    unsigned Align;
    if (ArgTy->isIntegerType())
      Align = 1;
    else
      Align = getContext().getTypeSizeInChars(ArgTy).getQuantity();
    SI->setAlignment(Align);
    return SI;
  }
  // 3DNow!
  case X86::BI__builtin_ia32_pswapdsf:
  case X86::BI__builtin_ia32_pswapdsi: {
    const char *name = 0;
    Intrinsic::ID ID = Intrinsic::not_intrinsic;
    switch(BuiltinID) {
    default: llvm_unreachable("Unsupported intrinsic!");
    case X86::BI__builtin_ia32_pswapdsf:
    case X86::BI__builtin_ia32_pswapdsi:
      name = "pswapd";
      ID = Intrinsic::x86_3dnowa_pswapd;
      break;
    }
    llvm::Type *MMXTy = llvm::Type::getX86_MMXTy(getLLVMContext());
    Ops[0] = Builder.CreateBitCast(Ops[0], MMXTy, "cast");
    llvm::Function *F = CGM.getIntrinsic(ID);
    return Builder.CreateCall(F, Ops, name);
  }
  case X86::BI__builtin_ia32_rdrand16_step:
  case X86::BI__builtin_ia32_rdrand32_step:
  case X86::BI__builtin_ia32_rdrand64_step:
  case X86::BI__builtin_ia32_rdseed16_step:
  case X86::BI__builtin_ia32_rdseed32_step:
  case X86::BI__builtin_ia32_rdseed64_step: {
    Intrinsic::ID ID;
    switch (BuiltinID) {
    default: llvm_unreachable("Unsupported intrinsic!");
    case X86::BI__builtin_ia32_rdrand16_step:
      ID = Intrinsic::x86_rdrand_16;
      break;
    case X86::BI__builtin_ia32_rdrand32_step:
      ID = Intrinsic::x86_rdrand_32;
      break;
    case X86::BI__builtin_ia32_rdrand64_step:
      ID = Intrinsic::x86_rdrand_64;
      break;
    case X86::BI__builtin_ia32_rdseed16_step:
      ID = Intrinsic::x86_rdseed_16;
      break;
    case X86::BI__builtin_ia32_rdseed32_step:
      ID = Intrinsic::x86_rdseed_32;
      break;
    case X86::BI__builtin_ia32_rdseed64_step:
      ID = Intrinsic::x86_rdseed_64;
      break;
    }

    Value *Call = Builder.CreateCall(CGM.getIntrinsic(ID));
    Builder.CreateStore(Builder.CreateExtractValue(Call, 0), Ops[0]);
    return Builder.CreateExtractValue(Call, 1);
  }
  // AVX2 broadcast
  case X86::BI__builtin_ia32_vbroadcastsi256: {
    Value *VecTmp = CreateMemTemp(E->getArg(0)->getType());
    Builder.CreateStore(Ops[0], VecTmp);
    Value *F = CGM.getIntrinsic(Intrinsic::x86_avx2_vbroadcasti128);
    return Builder.CreateCall(F, Builder.CreateBitCast(VecTmp, Int8PtrTy));
  }
  }
}


Value *CodeGenFunction::EmitPPCBuiltinExpr(unsigned BuiltinID,
                                           const CallExpr *E) {
  SmallVector<Value*, 4> Ops;

  for (unsigned i = 0, e = E->getNumArgs(); i != e; i++)
    Ops.push_back(EmitScalarExpr(E->getArg(i)));

  Intrinsic::ID ID = Intrinsic::not_intrinsic;

  switch (BuiltinID) {
  default: return 0;

  // vec_ld, vec_lvsl, vec_lvsr
  case PPC::BI__builtin_altivec_lvx:
  case PPC::BI__builtin_altivec_lvxl:
  case PPC::BI__builtin_altivec_lvebx:
  case PPC::BI__builtin_altivec_lvehx:
  case PPC::BI__builtin_altivec_lvewx:
  case PPC::BI__builtin_altivec_lvsl:
  case PPC::BI__builtin_altivec_lvsr:
  {
    Ops[1] = Builder.CreateBitCast(Ops[1], Int8PtrTy);

    Ops[0] = Builder.CreateGEP(Ops[1], Ops[0]);
    Ops.pop_back();

    switch (BuiltinID) {
    default: llvm_unreachable("Unsupported ld/lvsl/lvsr intrinsic!");
    case PPC::BI__builtin_altivec_lvx:
      ID = Intrinsic::ppc_altivec_lvx;
      break;
    case PPC::BI__builtin_altivec_lvxl:
      ID = Intrinsic::ppc_altivec_lvxl;
      break;
    case PPC::BI__builtin_altivec_lvebx:
      ID = Intrinsic::ppc_altivec_lvebx;
      break;
    case PPC::BI__builtin_altivec_lvehx:
      ID = Intrinsic::ppc_altivec_lvehx;
      break;
    case PPC::BI__builtin_altivec_lvewx:
      ID = Intrinsic::ppc_altivec_lvewx;
      break;
    case PPC::BI__builtin_altivec_lvsl:
      ID = Intrinsic::ppc_altivec_lvsl;
      break;
    case PPC::BI__builtin_altivec_lvsr:
      ID = Intrinsic::ppc_altivec_lvsr;
      break;
    }
    llvm::Function *F = CGM.getIntrinsic(ID);
    return Builder.CreateCall(F, Ops, "");
  }

  // vec_st
  case PPC::BI__builtin_altivec_stvx:
  case PPC::BI__builtin_altivec_stvxl:
  case PPC::BI__builtin_altivec_stvebx:
  case PPC::BI__builtin_altivec_stvehx:
  case PPC::BI__builtin_altivec_stvewx:
  {
    Ops[2] = Builder.CreateBitCast(Ops[2], Int8PtrTy);
    Ops[1] = Builder.CreateGEP(Ops[2], Ops[1]);
    Ops.pop_back();

    switch (BuiltinID) {
    default: llvm_unreachable("Unsupported st intrinsic!");
    case PPC::BI__builtin_altivec_stvx:
      ID = Intrinsic::ppc_altivec_stvx;
      break;
    case PPC::BI__builtin_altivec_stvxl:
      ID = Intrinsic::ppc_altivec_stvxl;
      break;
    case PPC::BI__builtin_altivec_stvebx:
      ID = Intrinsic::ppc_altivec_stvebx;
      break;
    case PPC::BI__builtin_altivec_stvehx:
      ID = Intrinsic::ppc_altivec_stvehx;
      break;
    case PPC::BI__builtin_altivec_stvewx:
      ID = Intrinsic::ppc_altivec_stvewx;
      break;
    }
    llvm::Function *F = CGM.getIntrinsic(ID);
    return Builder.CreateCall(F, Ops, "");
  }
  }
}
