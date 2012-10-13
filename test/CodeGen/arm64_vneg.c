// RUN: %clang -O1 -arch arm64 -S -o - -emit-llvm %s | FileCheck %s
// Test ARM64 SIMD negate and saturating negate intrinsics

#include <aarch64_simd.h>

int64x2_t test_vnegq_s64(int64x2_t a1) {
  // CHECK: test_vnegq_s64
  return vnegq_s64(a1);
  // CHECK: sub <2 x i64> zeroinitializer, %a1
  // CHECK-NEXT: ret
}

int64x2_t test_vqnegq_s64(int64x2_t a1) {
  // CHECK: test_vqnegq_s64
  return vqnegq_s64(a1);
  // CHECK: llvm.arm64.neon.sqneg.v2i64
  // CHECK-NEXT: ret
}
