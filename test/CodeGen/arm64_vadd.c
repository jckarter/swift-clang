// RUN: %clang -O1 -arch arm64 -S -o - -emit-llvm %s | FileCheck %s
// Test ARM64 SIMD add intrinsics

#include <aarch64_simd.h>

int64_t test_vaddlv_s32(int32x2_t a1) {
  // CHECK: test_vaddlv_s32
  return vaddlv_s32(a1);
  // CHECK: llvm.arm64.neon.saddlv.i64.v2i32
  // CHECK-NEXT: ret
}

uint64_t test_vaddlv_u32(uint32x2_t a1) {
  // CHECK: test_vaddlv_u32
  return vaddlv_u32(a1);
  // CHECK: llvm.arm64.neon.uaddlv.i64.v2i32
  // CHECK-NEXT: ret
}

int16_t test_vaddv_s16(int16x4_t a1) {
  // CHECK: test_vaddv_s16
  return vaddv_s16(a1);
  // CHECK: llvm.arm64.neon.addv.i32.v4i16
  // don't check for return here (there's a trunc?)
}

int32_t test_vaddv_s32(int32x2_t a1) {
  // CHECK: test_vaddv_s32
  return vaddv_s32(a1);
  // CHECK: llvm.arm64.neon.addv.i32.v2i32
  // CHECK-NEXT: ret
}

