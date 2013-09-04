//===--- arm_neon_common.td - ARM and ARM64 common definitions ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  The following definitions are shared between arm_neon.td and arm64_simd.td.
//
//===----------------------------------------------------------------------===//

class Op;

def OP_NONE  : Op;
def OP_UNAVAILABLE : Op;
def OP_ADD   : Op;
def OP_ADDL  : Op;
def OP_ADDL_HIGH : Op;
def OP_ADDW  : Op;
def OP_SUB   : Op;
def OP_SUBL  : Op;
def OP_SUBW  : Op;
def OP_MUL   : Op;
def OP_MLA   : Op;
def OP_MLAL  : Op;
def OP_MLS   : Op;
def OP_MLSL  : Op;
def OP_MUL_N : Op;
def OP_MLA_N : Op;
def OP_MLS_N : Op;
def OP_MLAL_N : Op;
def OP_MLSL_N : Op;
def OP_MOVL_HIGH : Op;
def OP_MUL_LN: Op;
def OP_MULL_LN : Op;
def OP_MULL_HIGH : Op;
def OP_MULL_HIGH_N : Op;
def OP_MULL_HIGH_LN : Op;
def OP_MLA_LN: Op;
def OP_MLS_LN: Op;
def OP_MLAL_LN : Op;
def OP_MLAL_HIGH : Op;
def OP_MLAL_HIGH_LN : Op;
def OP_MLAL_HIGH_N : Op;
def OP_MLSL_LN : Op;
def OP_MLSL_HIGH : Op;
def OP_MLSL_HIGH_LN : Op;
def OP_MLSL_HIGH_N : Op;
def OP_QDMULL_LN : Op;
def OP_QDMULL_HIGH_N : Op;
def OP_QDMULL_HIGH_LN : Op;
def OP_QDMLAL_LN : Op;
def OP_QDMLAL_HIGH_LN : Op;
def OP_QDMLAL_HIGH_N : Op;
def OP_QDMLSL_LN : Op;
def OP_QDMLSL_HIGH_LN : Op;
def OP_QDMLSL_HIGH_N : Op;
def OP_QDMULH_LN : Op;
def OP_QRDMULH_LN : Op;
def OP_EQ    : Op;
def OP_GE    : Op;
def OP_LE    : Op;
def OP_GT    : Op;
def OP_LT    : Op;
def OP_NEG   : Op;
def OP_NOT   : Op;
def OP_AND   : Op;
def OP_OR    : Op;
def OP_XOR   : Op;
def OP_ANDN  : Op;
def OP_ORN   : Op;
def OP_CAST  : Op;
def OP_HI    : Op;
def OP_LO    : Op;
def OP_CONC  : Op;
def OP_DUP   : Op;
def OP_DUP_LN: Op;
def OP_SEL   : Op;
def OP_REV64 : Op;
def OP_REV32 : Op;
def OP_REV16 : Op;
def OP_REINT : Op;
def OP_ABDL  : Op;
def OP_ABA   : Op;
def OP_ABAL  : Op;
def OP_QDMLAL : Op;
def OP_QDMLSL : Op;
def OP_QDMLSL_N : Op;
def OP_QDMLAL_N : Op;
def OP_SHL : Op;
def OP_SHR : Op;
def OP_DIV : Op;
def OP_LONG_HI : Op;
def OP_NARROW_HI : Op;
def OP_MOVL_HI : Op;

class Inst <string n, string p, string t, Op o> {
  string Name = n;
  string Prototype = p;
  string Types = t;
  Op Operand = o;
  bit isShift = 0;
  bit isVCVT_N = 0;
  bit isA64 = 0;

  // Certain intrinsics have different names than their representative
  // instructions. This field allows us to handle this correctly when we
  // are generating tests.
  string InstName = "";

  // Certain intrinsics even though they are not a WOpInst or LOpInst,
  // generate a WOpInst/LOpInst instruction (see below for definition
  // of a WOpInst/LOpInst). For testing purposes we need to know
  // this. Ex: vset_lane which outputs vmov instructions.
  bit isHiddenWInst = 0;
  bit isHiddenLInst = 0;
}

// The following instruction classes are implemented via builtins.
// These declarations are used to generate Builtins.def:
//
// SInst: Instruction with signed/unsigned suffix (e.g., "s8", "u8", "p8")
// IInst: Instruction with generic integer suffix (e.g., "i8")
// WInst: Instruction with only bit size suffix (e.g., "8")
class SInst<string n, string p, string t> : Inst<n, p, t, OP_NONE> {}
class IInst<string n, string p, string t> : Inst<n, p, t, OP_NONE> {}
class WInst<string n, string p, string t> : Inst<n, p, t, OP_NONE> {}

// The following instruction classes are implemented via operators
// instead of builtins. As such these declarations are only used for
// the purpose of generating tests.
//
// SOpInst:       Instruction with signed/unsigned suffix (e.g., "s8",
//                "u8", "p8").
// IOpInst:       Instruction with generic integer suffix (e.g., "i8").
// WOpInst:       Instruction with bit size only suffix (e.g., "8").
// LOpInst:       Logical instruction with no bit size suffix.
// NoTestOpInst:  Intrinsic that has no corresponding instruction.
class SOpInst<string n, string p, string t, Op o> : Inst<n, p, t, o> {}
class IOpInst<string n, string p, string t, Op o> : Inst<n, p, t, o> {}
class WOpInst<string n, string p, string t, Op o> : Inst<n, p, t, o> {}
class LOpInst<string n, string p, string t, Op o> : Inst<n, p, t, o> {}
class NoTestOpInst<string n, string p, string t, Op o> : Inst<n, p, t, o> {}

// prototype: return (arg, arg, ...)
// v: void
// t: best-fit integer (int/poly args)
// x: signed integer   (int/float args)
// u: unsigned integer (int/float args)
// f: float (int args)
// d: default
// g: default, ignore 'Q' size modifier.
// w: double width elements, same num elts
// n: double width elements, half num elts
// h: half width elements, double num elts
// e: half width elements, double num elts, unsigned
// b: half width elements, same num elts
// i: constant int
// l: constant uint64
// s: scalar of element type
// a: scalar of element type (splat to vector type)
// m: half width elements, same num elts
// o: scalar of signed int type (same size as float arg elements)
// r: scalar of unsigned int type (same size as float arg elements)
// q: scalar of half element type width
// z: scalar of double element type width
// k: default elt width, double num elts
// j: default elt width, half num elts
// #: array of default vectors
// p: pointer type
// c: const pointer type

// FIXME: The ARM64 generator uses a different mapping for some parameter types:
// m: scalar of float type (same size as int arg elements)

// sizes:
// c: char
// s: short
// i: int
// l: long
// d: double
// f: float
// h: half-float

// size modifiers:
// S: scalar, only used for function mangling.
// U: unsigned
// Q: 128b
// H: 128b without mangling 'q'
// P: polynomial
