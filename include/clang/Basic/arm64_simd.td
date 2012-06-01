//===--- arm64_simd.td - ARM64 SIMD compiler interface --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the TableGen definitions from which the ARM64 SIMD header
//  file will be generated.
//
//===----------------------------------------------------------------------===//

include "clang/Basic/arm_neon_common.td"

////////////////////////////////////////////////////////////////////////////////
// Addition
def VADD : Inst<"vadd", "ddd", "csilfUcUsUiUlQcQsQiQlQfQUcQUsQUiQUl", OP_ADD>;
def VADDL   : Inst<"vaddl", "wdd", "csiUcUsUi", OP_ADDL>;
def VADDW   : Inst<"vaddw", "wwd", "csiUcUsUi", OP_ADDW>;
def VHADD   : SInst<"vhadd", "ddd", "csiUcUsUiQcQsQiQUcQUsQUi">;
def VRHADD  : SInst<"vrhadd", "ddd", "csiUcUsUiQcQsQiQUcQUsQUi">;
def VQADD   : SInst<"vqadd", "ddd", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl">;
def VUSQADD : IInst<"vusqadd", "ddd", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl">;
def VADDHN  : IInst<"vaddhn", "hkk", "silUsUiUl">;
def VRADDHN : IInst<"vraddhn", "hkk", "silUsUiUl">;
def VADDL_HIGH : SInst<"vaddl_high", "ndd", "RcRsRiRUcRUsRUi">;
def VADDW_HIGH : SInst<"vaddw_high", "ndd", "RcRsRiRUcRUsRUi">;
def VADDHN_HIGH  : SInst<"vaddhn_high",  "hbdd", "RsRiRlRUsRUiRUl">;
def VRADDHN_HIGH : SInst<"vraddhn_high", "hbdd", "RsRiRlRUsRUiRUl">;

////////////////////////////////////////////////////////////////////////////////
// Multiplication
def VMUL     : Inst<"vmul", "ddd", "csifUcUsUiQcQsQiQfQUcQUsQUi", OP_MUL>;
def VMULP    : SInst<"vmul", "ddd", "PcQPc">;
def VMLA     : Inst<"vmla", "dddd", "csifUcUsUiQcQsQiQfQUcQUsQUi", OP_MLA>;
def VFMA     : IInst<"vfma", "dddd", "fQf">;
def VMLAL    : Inst<"vmlal", "wwdd", "csiUcUsUi", OP_MLAL>;
def VMLS     : Inst<"vmls", "dddd", "csifUcUsUiQcQsQiQfQUcQUsQUi", OP_MLS>;
def VFMS     : IInst<"vfms", "dddd", "fQf">;
def VMLSL    : Inst<"vmlsl", "wwdd", "csiUcUsUi", OP_MLSL>;
def VQDMULH  : SInst<"vqdmulh", "ddd", "siQsQi">;
def VQRDMULH : SInst<"vqrdmulh", "ddd", "siQsQi">;
def VQDMLAL  : Inst<"vqdmlal", "wwdd", "si", OP_QDMLAL>;
def VQDMLSL  : Inst<"vqdmlsl", "wwdd", "si", OP_QDMLSL>;
def VMULL    : SInst<"vmull", "wdd", "csiUcUsUiPc">;
def VQDMULL  : SInst<"vqdmull", "wdd", "si">;
def VMLAL_HIGH : SInst<"vmlal_high", "nndd", "RcRsRiRUcRUsRUi">;
def VMLSL_HIGH : SInst<"vmlsl_high", "nndd", "RcRsRiRUcRUsRUi">;
def VMULL_HIGH : SInst<"vmull_high", "ndd",  "RcRsRiRUcRUsRUi">;
def VQDMLAL_HIGH : SInst<"vqdmlal_high", "nndd", "RsRi">;
def VQDMLSL_HIGH : SInst<"vqdmlsl_high", "nndd", "RsRi">;
def VQDMULL_HIGH : SInst<"vqdmull_high", "ndd",  "RsRi">;
def VMLAL2_LANE  : SInst<"vmlal2_lane", "nnddi",  "RsRiRUsRUi">;
def VMLSL2_LANE  : SInst<"vmlsl2_lane", "nnddi",  "RsRiRUsRUi">;
def VMULL2_LANE  : SInst<"vmull2_lane", "nddi",  "RsRiRUsRUi">;
def VQDMLAL2_LANE  : SInst<"vqdmlal2_lane", "nnddi",  "RsRi">;
def VQDMLSL2_LANE  : SInst<"vqdmlsl2_lane", "nnddi",  "RsRi">;
def VQDMULL2_LANE  : SInst<"vqdmull2_lane", "nddi",  "RsRi">;

////////////////////////////////////////////////////////////////////////////////
// Miscellaneous FP
def VDIV  : SInst<"vdiv",  "ddd", "fQfQd">;
def VMULX : SInst<"vmulx", "ddd", "fQfQd">;
def VMAXNMP : SInst<"vpmaxnm", "ddd", "fQfQd">;
def VMINNMP : SInst<"vpminnm", "ddd", "fQfQd">;
def VSQRT : SInst<"vsqrt", "dd", "fQfQd">;

////////////////////////////////////////////////////////////////////////////////
// Subtraction
def VSUB : Inst<"vsub", "ddd", "csilfUcUsUiUlQcQsQiQlQfQUcQUsQUiQUl", OP_SUB>;
def VSUBL   : Inst<"vsubl", "wdd", "csiUcUsUi", OP_SUBL>;
def VSUBW   : Inst<"vsubw", "wwd", "csiUcUsUi", OP_SUBW>;
def VQSUB   : SInst<"vqsub", "ddd", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl">;
def VHSUB   : SInst<"vhsub", "ddd", "csiUcUsUiQcQsQiQUcQUsQUi">;
def VSUBHN  : IInst<"vsubhn", "hkk", "silUsUiUl">;
def VRSUBHN : IInst<"vrsubhn", "hkk", "silUsUiUl">;
def VSUBL_HIGH   : SInst<"vsubl_high",   "ndd", "RcRsRiRUcRUsRUi">;
def VSUBW_HIGH   : SInst<"vsubw_high",   "ndd", "RcRsRiRUcRUsRUi">;
def VRSUBHN_HIGH : SInst<"vrsubhn_high", "hbdd", "RsRiRlRUsRUiRUl">;
def VSUBHN_HIGH  : SInst<"vsubhn_high",   "hbdd", "RsRiRlRUsRUiRUl">;
def VSUBD : Inst<"vsubd", "sss", "lUl", OP_SUB>;
def VADDD : Inst<"vaddd", "sss", "lUl", OP_ADD>;

////////////////////////////////////////////////////////////////////////////////
// Comparison
def VCEQ  : Inst<"vceq", "udd", "csifUcUsUiPcQcQsQiQfQdQUcQUsQUiQPc", OP_EQ>;
def VCGE  : Inst<"vcge", "udd", "csifUcUsUiQcQsQiQfQdQUcQUsQUi", OP_GE>;
def VCLE  : Inst<"vcle", "udd", "csifUcUsUiQcQsQiQfQdQUcQUsQUi", OP_LE>;
def VCGT  : Inst<"vcgt", "udd", "csifUcUsUiQcQsQiQfQdQUcQUsQUi", OP_GT>;
def VCLT  : Inst<"vclt", "udd", "csifUcUsUiQcQsQiQfQdQUcQUsQUi", OP_LT>;
def VCAGE : IInst<"vcage", "udd", "fQf">;
def VCALE : IInst<"vcale", "udd", "fQf">;
def VCAGT : IInst<"vcagt", "udd", "fQf">;
def VCALT : IInst<"vcalt", "udd", "fQf">;
def VTST  : WInst<"vtst", "udd", "csiUcUsUiPcQcQsQiQUcQUsQUiQPc">;
// Scalar
def VCEQD : Inst<"vceqd", "sss", "lUl", OP_EQ>;
def VCLED : Inst<"vcled", "sss", "lUl", OP_LE>;
def VCGTD : Inst<"vcgtd", "sss", "lUl", OP_GT>;
def VTSTD : SInst<"vtstd", "sss", "lUl">;
// Scalar Compare to Zero.
def VCEQZD : SInst<"vceqzd", "ss", "lUl">;
def VCGEZD : SInst<"vcgezd", "ss", "lUl">;
def VCLEZD : SInst<"vclezd", "ss", "lUl">;
def VCGTZD : SInst<"vcgtzd", "ss", "l">;
def VCLTZD : SInst<"vcltzd", "ss", "l">;

////////////////////////////////////////////////////////////////////////////////
// Absolute Difference
def VABD  : SInst<"vabd", "ddd",  "csiUcUsUifQcQsQiQUcQUsQUiQf">;
def VABDL : IInst<"vabdl", "wdd",  "csiUcUsUi">;
def VABA  : Inst<"vaba", "dddd", "csiUcUsUiQcQsQiQUcQUsQUi", OP_ABA>;
def VABAL : SInst<"vabal", "wwdd", "csiUcUsUi">;
def VABAL_HIGH : SInst<"vabal_high", "nndd", "RcRsRiRUcRUsRUi">;
def VABDL_HIGH : SInst<"vabdl_high", "ndd",  "RcRsRiRUcRUsRUi">;
def VABDS : SInst<"vabds", "sss", "f">;
def VABDD : SInst<"vabdd", "sss", "d">;

////////////////////////////////////////////////////////////////////////////////
// Max/Min
def VMAX : SInst<"vmax", "ddd", "csiUcUsUifQcQsQiQUcQUsQUiQf">;
def VMIN : SInst<"vmin", "ddd", "csiUcUsUifQcQsQiQUcQUsQUiQf">;

////////////////////////////////////////////////////////////////////////////////
// Max/Min Number
def VMAXNM : SInst<"vmaxnm", "ddd", "fQfQd">;
def VMINNM : SInst<"vminnm", "ddd", "fQfQd">;

////////////////////////////////////////////////////////////////////////////////
// Pairwise Addition
def VPADD  : IInst<"vpadd", "ddd", "csiUcUsUif">;
def VPADDL : SInst<"vpaddl", "nd",  "csiUcUsUiQcQsQiQUcQUsQUi">;
def VPADAL : SInst<"vpadal", "nnd", "csiUcUsUiQcQsQiQUcQUsQUi">;

////////////////////////////////////////////////////////////////////////////////
// Folding Max/Min
def VPMAX : SInst<"vpmax", "ddd", "csiUcUsUifQcQsQiQUcQUsQUiQfQd">;
def VPMIN : SInst<"vpmin", "ddd", "csiUcUsUifQcQsQiQUcQUsQUiQfQd">;

////////////////////////////////////////////////////////////////////////////////
// Reciprocal/Sqrt
def VRECPS  : IInst<"vrecps", "ddd", "fQfQd">;
def VRSQRTS : IInst<"vrsqrts", "ddd", "fQfQd">;

////////////////////////////////////////////////////////////////////////////////
// Shifts by signed variable
def VSHL   : SInst<"vshl", "ddx", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl">;
def VQSHL  : SInst<"vqshl", "ddx", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl">;
def VRSHL  : SInst<"vrshl", "ddx", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl">;
def VQRSHL : SInst<"vqrshl", "ddx", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl">;

////////////////////////////////////////////////////////////////////////////////
// Shifts by constant
let isShift = 1 in {
def VSHR_N     : SInst<"vshr_n", "ddi", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl">;
def VSHL_N     : IInst<"vshl_n", "ddi", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl">;
def VRSHR_N    : SInst<"vrshr_n", "ddi", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl">;
def VSRA_N     : SInst<"vsra_n", "dddi", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl">;
def VRSRA_N    : SInst<"vrsra_n", "dddi", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl">;
def VQSHL_N    : SInst<"vqshl_n", "ddi", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl">;
def VQSHLU_N   : SInst<"vqshlu_n", "udi", "csilQcQsQiQl">;
def VSHRN_N    : IInst<"vshrn_n", "hki", "silUsUiUl">;
def VQSHRUN_N  : SInst<"vqshrun_n", "eki", "sil">;
def VQRSHRUN_N : SInst<"vqrshrun_n", "eki", "sil">;
def VQSHRN_N   : SInst<"vqshrn_n", "hki", "silUsUiUl">;
def VRSHRN_N   : IInst<"vrshrn_n", "hki", "silUsUiUl">;
def VQRSHRN_N  : SInst<"vqrshrn_n", "hki", "silUsUiUl">;
def VSHLL_N    : SInst<"vshll_n", "wdi", "csiUcUsUi">;
def VRSHRN_HIGH : SInst<"vrshrn_high_n", "hhki", "RsRiRlRUsRUiRUl">;
def VSHRN_HIGH  : SInst<"vshrn_high_n",  "hhki", "RsRiRlRUsRUiRUl">;

////////////////////////////////////////////////////////////////////////////////
// Shifts with insert
def VSRI_N : WInst<"vsri_n", "dddi",
                   "csilUcUsUiUlPcPsQcQsQiQlQUcQUsQUiQUlQPcQPs">;
def VSLI_N : WInst<"vsli_n", "dddi",
                   "csilUcUsUiUlPcPsQcQsQiQlQUcQUsQUiQUlQPcQPs">;
}

////////////////////////////////////////////////////////////////////////////////
// Vector FP->Int Conversions

// F32->Integer
def VCVT_S32     : SInst<"vcvt_s32", "xd",  "fQf">;
def VCVT_U32     : SInst<"vcvt_u32", "ud",  "fQf">;
// Integer->F32
def VCVT_F32     : SInst<"vcvt_f32", "fd", "iUiQiQUi">;
// F16->F32
def VCVT_F32_F16 : SInst<"vcvt_f32_f16", "fd", "h">;
// F32->F16
def VCVT_F16     : SInst<"vcvt_f16", "hk",  "f">;
let isVCVT_N = 1 in {
// f32 -> signed-fixed-point
def VCVT_N_S32   : SInst<"vcvt_n_s32", "xdi", "fQf">;
// f32 -> unsigned-fixed-point
def VCVT_N_U32   : SInst<"vcvt_n_u32", "udi", "fQf">;
// fixed point -> f32
def VCVT_N_F32   : SInst<"vcvt_n_f32", "fdi", "iUiQiQUi">;
// f64 -> signed-fixed-point
def VCVT_N_S64   : SInst<"vcvt_n_s64", "xdi", "Qd">;
// f64 -> unsigned-fixed-point
def VCVT_N_U64   : SInst<"vcvt_n_u64", "udi", "Qd">;
// fixed point -> f64
def VCVT_N_F64   : SInst<"vcvtq_n_f64", "fdi", "RlRUl">;
}
// f32/f64 -> integer w/ rounding mode.
def VCVTA_S32 : SInst<"vcvta_s32", "xd", "fQf">;
def VCVTA_U32 : SInst<"vcvta_u32", "ud", "fQf">;
def VCVTA_S64 : SInst<"vcvta_s64", "xd", "Qd">;
def VCVTA_U64 : SInst<"vcvta_u64", "ud", "Qd">;
def VCVTM_S32 : SInst<"vcvtm_s32", "xd", "fQf">;
def VCVTM_U32 : SInst<"vcvtm_u32", "ud", "fQf">;
def VCVTM_S64 : SInst<"vcvtm_s64", "xd", "Qd">;
def VCVTM_U64 : SInst<"vcvtm_u64", "ud", "Qd">;
def VCVTN_S32 : SInst<"vcvtn_s32", "xd", "fQf">;
def VCVTN_U32 : SInst<"vcvtn_u32", "ud", "fQf">;
def VCVTN_S64 : SInst<"vcvtn_s64", "xd", "Qd">;
def VCVTN_U64 : SInst<"vcvtn_u64", "ud", "Qd">;
def VCVTP_S32 : SInst<"vcvtp_s32", "xd", "fQf">;
def VCVTP_U32 : SInst<"vcvtp_u32", "ud", "fQf">;
def VCVTP_S64 : SInst<"vcvtp_s64", "xd", "Qd">;
def VCVTP_U64 : SInst<"vcvtp_u64", "ud", "Qd">;
def VCVTZ_S32 : SInst<"vcvtz_s32", "xd", "fQf">;
def VCVTZ_U32 : SInst<"vcvtz_u32", "ud", "fQf">;
def VCVTZ_S64 : SInst<"vcvtz_s64", "xd", "Qd">;
def VCVTZ_U64 : SInst<"vcvtz_u64", "ud", "Qd">;
//let isShift = 1 in {
//def VCVTI_M : SInst<"vcvti_m", "fdi", "iUiQiQUiQlQUl">;
//def VCVTI_N : SInst<"vcvti_n", "fdi", "iUiQiQUiQlQUl">;
//def VCVTI_P : SInst<"vcvti_p", "fdi", "iUiQiQUiQlQUl">;
//def VCVTI_Z : SInst<"vcvti_z", "fdi", "iUiQiQUiQlQUl">;
//}
//def VCVT  : SInst<"vcvt_f32",  "bd", "Rd">;
//def VCVT2 : SInst<"vcvt2_f32", "hhd", "Rd">;

def VRND_A : SInst<"vrnda", "dd", "f">;
def VRND_I : SInst<"vrndi", "dd", "f">;
def VRND_M : SInst<"vrndm", "dd", "f">;
def VRND_N : SInst<"vrndn", "dd", "f">;
def VRND_P : SInst<"vrndp", "dd", "f">;
def VRND_X : SInst<"vrndx", "dd", "f">;
def VRND_Z : SInst<"vrndz", "dd", "f">;
def VRNDQ_A : SInst<"vrndqa", "dd", "RfRd">;
def VRNDQ_I : SInst<"vrndqi", "dd", "RfRd">;
def VRNDQ_M : SInst<"vrndqm", "dd", "RfRd">;
def VRNDQ_N : SInst<"vrndqn", "dd", "RfRd">;
def VRNDQ_P : SInst<"vrndqp", "dd", "RfRd">;
def VRNDQ_X : SInst<"vrndqx", "dd", "RfRd">;
def VRNDQ_Z : SInst<"vrndqz", "dd", "RfRd">;

////////////////////////////////////////////////////////////////////////////////
// Loads and stores of a single vector
def VLD1      : WInst<"vld1", "dc",
                      "QUcQUsQUiQUlQcQsQiQlQhQfQPcQPsUcUsUiUlcsilhfPcPs">;
def VLD1_LANE : WInst<"vld1_lane", "dcdi",
                      "QUcQUsQUiQUlQcQsQiQlQhQfQPcQPsUcUsUiUlcsilhfPcPs">;
def VLD1_DUP  : WInst<"vld1_dup", "dc",
                      "QUcQUsQUiQUlQcQsQiQlQhQfQPcQPsUcUsUiUlcsilhfPcPs">;
def VST1      : WInst<"vst1", "vpd",
                      "QUcQUsQUiQUlQcQsQiQlQhQfQPcQPsUcUsUiUlcsilhfPcPs">;
def VST1_LANE : WInst<"vst1_lane", "vpdi",
                      "QUcQUsQUiQUlQcQsQiQlQhQfQPcQPsUcUsUiUlcsilhfPcPs">;

////////////////////////////////////////////////////////////////////////////////
// Loads and stores of an N-element structure
def VLD2 : WInst<"vld2", "2c", "QUcQUsQUiQcQsQiQhQfQPcQPsUcUsUicsihfPcPs">;
def VLD3 : WInst<"vld3", "3c", "QUcQUsQUiQcQsQiQhQfQPcQPsUcUsUicsihfPcPs">;
def VLD4 : WInst<"vld4", "4c", "QUcQUsQUiQcQsQiQhQfQPcQPsUcUsUicsihfPcPs">;
def VLD2_DUP  : WInst<"vld2_dup", "2c", "UcUsUiUlcsilhfPcPs">;
def VLD3_DUP  : WInst<"vld3_dup", "3c", "UcUsUiUlcsilhfPcPs">;
def VLD4_DUP  : WInst<"vld4_dup", "4c", "UcUsUiUlcsilhfPcPs">;
def VLD2_LANE : WInst<"vld2_lane", "2c2i", "QUsQUiQsQiQhQfQPsUcUsUicsihfPcPs">;
def VLD3_LANE : WInst<"vld3_lane", "3c3i", "QUsQUiQsQiQhQfQPsUcUsUicsihfPcPs">;
def VLD4_LANE : WInst<"vld4_lane", "4c4i", "QUsQUiQsQiQhQfQPsUcUsUicsihfPcPs">;
def VST2 : WInst<"vst2", "vp2", "QUcQUsQUiQUlQcQsQiQlQhQfQPcQPsUcUsUicsihfPcPs">;
def VST3 : WInst<"vst3", "vp3", "QUcQUsQUiQUlQcQsQiQlQhQfQPcQPsUcUsUicsihfPcPs">;
def VST4 : WInst<"vst4", "vp4", "QUcQUsQUiQUlQcQsQiQlQhQfQPcQPsUcUsUicsihfPcPs">;
def VST2_LANE : WInst<"vst2_lane", "vp2i", "QUsQUiQsQiQhQfQPsUcUsUicsihfPcPs">;
def VST3_LANE : WInst<"vst3_lane", "vp3i", "QUsQUiQsQiQhQfQPsUcUsUicsihfPcPs">;
def VST4_LANE : WInst<"vst4_lane", "vp4i", "QUsQUiQsQiQhQfQPsUcUsUicsihfPcPs">;

////////////////////////////////////////////////////////////////////////////////
// Extract lanes from a vector
def VGET_LANE : SInst<"vget_lane", "sdi",
                      "csilUcUsUiUlfPcPsQcQsQiQlQUcQUsQUiQUlQfQPcQPs">;
// vdup<bhsd>_lane* is a synonym for vget_lane.
def VDUPB_LANE : SInst<"vdupb_lane", "sdi", "RcRUc">;
def VDUPH_LANE : SInst<"vduph_lane", "sdi", "RsRUs">;
def VDUPS_LANE : SInst<"vdups_lane", "sdi", "RiRUi">;
def VDUPD_LANE : SInst<"vdupd_lane", "sdi", "RlRUl">;

////////////////////////////////////////////////////////////////////////////////
// Set lanes within a vector
def VSET_LANE : IInst<"vset_lane", "dsdi",
                      "UcUsUicsiPcPsfQUcQUsQUiQcQsQiQPcQPsQflUlQlQUl">;

////////////////////////////////////////////////////////////////////////////////
// Initialize a vector from bit pattern
def VCREATE: Inst<"vcreate", "dl", "csihfUcUsUiUlPcPsl", OP_CAST>;

////////////////////////////////////////////////////////////////////////////////
// Set all lanes to same value
def VDUP_N : Inst<"vdup_n", "ds",
                  "UcUsUicsiPcPsfQUcQUsQUiQcQsQiQPcQPsQflUlQlQUl", OP_DUP>;
def VMOV_N : Inst<"vmov_n", "ds",
                  "UcUsUicsiPcPsfQUcQUsQUiQcQsQiQPcQPsQflUlQlQUl", OP_DUP>;
def VDUP_LANE : Inst<"vdup_lane", "dgi",
                     "UcUsUicsiPcPsfQUcQUsQUiQcQsQiQPcQPsQfQlQUl",OP_DUP_LN>;

////////////////////////////////////////////////////////////////////////////////
// Combining vectors
def VCOMBINE : Inst<"vcombine", "kdd", "csilhfUcUsUiUlPcPs", OP_CONC>;

////////////////////////////////////////////////////////////////////////////////
// Splitting vectors
def VGET_HIGH : Inst<"vget_high", "dk", "csilhfUcUsUiUlPcPs", OP_HI>;
def VGET_LOW  : Inst<"vget_low", "dk", "csilhfUcUsUiUlPcPs", OP_LO>;

////////////////////////////////////////////////////////////////////////////////
// Converting vectors
def VMOVN        : IInst<"vmovn", "hk",  "silUsUiUl">;
def VMOVL        : SInst<"vmovl", "wd",  "csiUcUsUi">;
def VQMOVN       : SInst<"vqmovn", "hk",  "silUsUiUl">;
def VQMOVUN      : SInst<"vqmovun", "ek",  "sil">;
def VXTL_HIGH    : SInst<"vxtl_high",   "nd", "RcRsRiRUcRUsRUi">;
def VQXTN_HIGH   : SInst<"vqxtn_high",  "hhd", "RsRiRlRUsRUiRUl">;
def VQXTUN_HIGH  : SInst<"vqxtun_high", "hhd", "RsRiRl">;
def VMOVN_HIGH   : SInst<"vmovn_high",  "hhd", "RsRiRlRUsRUiRUl">;
def VMOVL_HIGH   : SInst<"vmovl_high",  "nd", "RcRsRiRUcRUsRUi">;

////////////////////////////////////////////////////////////////////////////////
// Table lookup, Extended table lookup
def VTBL1  : Inst<"vtbl1", "ddd",  "UccPc", OP_UNAVAILABLE>;
def VTBL2  : Inst<"vtbl2", "d2d",  "UccPc", OP_UNAVAILABLE>;
def VTBL3  : Inst<"vtbl3", "d3d",  "UccPc", OP_UNAVAILABLE>;
def VTBL4  : Inst<"vtbl4", "d4d",  "UccPc", OP_UNAVAILABLE>;
def VTBL1Q : WInst<"vtbl1", "gdg",  "QUcQcQPc">;
def VTBL2Q : WInst<"vtbl2", "g2g",  "QUcQcQPc">;
def VTBL3Q : WInst<"vtbl3", "g3g",  "QUcQcQPc">;
def VTBL4Q : WInst<"vtbl4", "g4g",  "QUcQcQPc">;
def VQTBL1 : WInst<"vqtbl1", "ddd", "QUcQcQPc">;
def VQTBL2 : WInst<"vqtbl2", "d2d", "QUcQcQPc">;
def VQTBL3 : WInst<"vqtbl3", "d3d", "QUcQcQPc">;
def VQTBL4 : WInst<"vqtbl4", "d4d", "QUcQcQPc">;

def VTBX1  : Inst<"vtbx1", "dd1d",  "UccPc", OP_UNAVAILABLE>;
def VTBX2  : Inst<"vtbx2", "dd2d",  "UccPc", OP_UNAVAILABLE>;
def VTBX3  : Inst<"vtbx3", "dd3d",  "UccPc", OP_UNAVAILABLE>;
def VTBX4  : Inst<"vtbx4", "dd4d",  "UccPc", OP_UNAVAILABLE>;

def VTBX1Q  : WInst<"vtbx1",  "gg1g", "QUcQcQPc">;
def VTBX2Q  : WInst<"vtbx2",  "gg2g", "QUcQcQPc">;
def VTBX3Q  : WInst<"vtbx3",  "gg3g", "QUcQcQPc">;
def VTBX4Q  : WInst<"vtbx4",  "gg4g", "QUcQcQPc">;
def VQTBX1Q : WInst<"vqtbx1", "dddd", "QUcQcQPc">;
def VQTBX2Q : WInst<"vqtbx2", "dd2d", "QUcQcQPc">;
def VQTBX3Q : WInst<"vqtbx3", "dd3d", "QUcQcQPc">;
def VQTBX4Q : WInst<"vqtbx4", "dd4d", "QUcQcQPc">;

////////////////////////////////////////////////////////////////////////////////
// Vector Shuffles
def VTRN1 : WInst<"vtrn1", "ddd", "csiUcUsUiQcQsQiQlQUcQUsQUiQUl">;
def VTRN2 : WInst<"vtrn2", "ddd", "csiUcUsUiQcQsQiQlQUcQUsQUiQUl">;
def VZIP1 : WInst<"vzip1", "ddd", "csiUcUsUifQcQsQiQlQUcQUsQUiQUlQfQd">;
def VZIP2 : WInst<"vzip2", "ddd", "csiUcUsUifQcQsQiQlQUcQUsQUiQUlQfQd">;
def VUZP1 : WInst<"vuzp1", "ddd", "csiUcUsUifQcQsQiQlQUcQUsQUiQUlQfQd">;
def VUZP2 : WInst<"vuzp2", "ddd", "csiUcUsUifQcQsQiQlQUcQUsQUiQUlQfQd">;

////////////////////////////////////////////////////////////////////////////////
// Operations with a scalar value
def VMLA_LANE    : Inst<"vmla_lane", "dddgi", "siUsUiQsQiQUsQUi", OP_MLA_LN>;
def VFMA_LANE    : IInst<"vfma_lane", "ddddi", "fQfQd">;
def VMLAL_LANE   : Inst<"vmlal_lane", "wwddi", "siUsUi", OP_MLAL_LN>;
def VQDMLAL_LANE : Inst<"vqdmlal_lane", "wwddi", "si", OP_QDMLAL_LN>;
def VMLS_LANE    : Inst<"vmls_lane", "ddddi", "siUsUiQsQiQUsQUi", OP_MLS_LN>;
def VFMS_LANE    : IInst<"vfms_lane", "ddddi", "fQf">;
def VMLSL_LANE   : Inst<"vmlsl_lane", "wwddi", "siUsUi", OP_MLSL_LN>;
def VQDMLSL_LANE : Inst<"vqdmlsl_lane", "wwddi", "si", OP_QDMLSL_LN>;
def VMUL_N       : Inst<"vmul_n", "dds", "sifUsUiQsQiQfQUsQUi", OP_MUL_N>;
def VMUL_LANE    : Inst<"vmul_lane", "dddi", "sifUsUiQsQiQfQdQUsQUi", OP_MUL_LN>;
def VMULL_N      : SInst<"vmull_n", "wda", "siUsUi">;
def VMULL_LANE   : Inst<"vmull_lane", "wddi", "siUsUi", OP_MULL_LN>;
def VQDMULL_N    : SInst<"vqdmull_n", "wda", "si">;
def VQDMULL_LANE : Inst<"vqdmull_lane", "wddi", "si", OP_QDMULL_LN>;
def VQDMULH_N    : SInst<"vqdmulh_n", "dda", "siQsQi">;
def VQDMULH_LANE : Inst<"vqdmulh_lane", "ddgi", "siQsQi", OP_QDMULH_LN>;
def VQRDMULH_N   : SInst<"vqrdmulh_n", "dda", "siQsQi">;
def VQRDMULH_LANE : Inst<"vqrdmulh_lane", "ddgi", "siQsQi", OP_QRDMULH_LN>;
def VMLA_N       : Inst<"vmla_n", "ddda", "siUsUiQsQiQUsQUi", OP_MLA_N>;
def VFMA_N       : IInst<"vfma_n", "ddda", "fQf">;
def VMLAL_N      : Inst<"vmlal_n", "wwda", "siUsUi", OP_MLAL_N>;
def VQDMLAL_N    : Inst<"vqdmlal_n", "wwda", "si", OP_QDMLAL_N>;
def VMLS_N       : Inst<"vmls_n", "ddds", "siUsUiQsQiQUsQUi", OP_MLS_N>;
def VMLSL_N      : Inst<"vmlsl_n", "wwda", "siUsUi", OP_MLSL_N>;
def VQDMLSL_N    : Inst<"vqdmlsl_n", "wwda", "si",OP_QDMLSL_N>;
def VMULX_LANE   : SInst<"vmulx_lane", "dddi", "fQfQd">;

////////////////////////////////////////////////////////////////////////////////
// Cross Lane
def ADDV  : SInst<"vaddv",  "sd", "csUcUsQcQsQiQUcQUsQUi">;
def ADDLV : SInst<"vaddlv", "zd", "csUcUsQcQsQiQUcQUsQUi">;
def MAXV  : SInst<"vmaxv",  "sd", "csUcUsQcQsQiQUcQUsQUiQf">;
def MINV  : SInst<"vminv",  "sd", "csUcUsQcQsQiQUcQUsQUiQf">;
def MAXNMV : SInst<"vmaxnmv", "sd", "Qf">;
def MINMV  : SInst<"vminnmv", "sd", "Qf">;

////////////////////////////////////////////////////////////////////////////////
// Vector Extract
def VEXT : WInst<"vext", "dddi",
                 "cUcPcsUsPsiUilUlfQcQUcQPcQsQUsQPsQiQUiQlQUlQf">;

////////////////////////////////////////////////////////////////////////////////
// Reverse vector elements
def VREV64 : Inst<"vrev64", "dd", "csiUcUsUiPcPsfQcQsQiQUcQUsQUiQPcQPsQf",
                  OP_REV64>;
def VREV32 : Inst<"vrev32", "dd", "csUcUsPcPsQcQsQUcQUsQPcQPs", OP_REV32>;
def VREV16 : Inst<"vrev16", "dd", "cUcPcQcQUcQPc", OP_REV16>;

////////////////////////////////////////////////////////////////////////////////
// Other single operand arithmetic
def VABS    : SInst<"vabs", "dd", "csifQcQsQiQfQd">;
def VQABS   : SInst<"vqabs", "dd", "csiQcQsQi">;
def VNEG    : Inst<"vneg", "dd", "csifQcQsQiQfQd", OP_NEG>;
def VQNEG   : SInst<"vqneg", "dd", "csiQcQsQi">;
def VCLS    : SInst<"vcls", "dd", "csiQcQsQi">;
def VCLZ    : IInst<"vclz", "dd", "csiUcUsUiQcQsQiQUcQUsQUi">;
def VCNT    : WInst<"vcnt", "dd", "UccPcQUcQcQPc">;
def VRECPE  : SInst<"vrecpe", "dd",  "fUiQfQdQUi">;
def VRSQRTE : SInst<"vrsqrte", "dd", "fUiQfQdQUi">;
def VRBIT   : SInst<"vrbit", "dd", "cUcQcQUc">;

////////////////////////////////////////////////////////////////////////////////
// Logical operations
def VMVN : Inst<"vmvn", "dd", "csiUcUsUiPcQcQsQiQUcQUsQUiQPc", OP_NOT>;
def VAND : Inst<"vand", "ddd", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl", OP_AND>;
def VORR : Inst<"vorr", "ddd", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl", OP_OR>;
def VEOR : Inst<"veor", "ddd", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl", OP_XOR>;
def VBIC : Inst<"vbic", "ddd", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl", OP_ANDN>;
def VORN : Inst<"vorn", "ddd", "csilUcUsUiUlQcQsQiQlQUcQUsQUiQUl", OP_ORN>;
def VBSL : Inst<"vbsl", "dudd",
                "csilUcUsUiUlfPcPsQcQsQiQlQUcQUsQUiQUlQfQdQPcQPs", OP_SEL>;

////////////////////////////////////////////////////////////////////////////////
// Transposition operations
def VTRN : WInst<"vtrn", "2dd", "csiUcUsUifPcPsQcQsQiQUcQUsQUiQfQPcQPs">;
def VZIP : WInst<"vzip", "2dd", "csiUcUsUifPcPsQcQsQiQUcQUsQUiQfQPcQPs">;
def VUZP : WInst<"vuzp", "2dd", "csiUcUsUifPcPsQcQsQiQUcQUsQUiQfQPcQPs">;

////////////////////////////////////////////////////////////////////////////////
// Vector reinterpret cast operations
def VREINTERPRET
  : Inst<"vreinterpret", "dd",
         "csilUcUsUiUlhfPcPsQcQsQiQlQUcQUsQUiQUlQhQfQPcQPs", OP_REINT>;
