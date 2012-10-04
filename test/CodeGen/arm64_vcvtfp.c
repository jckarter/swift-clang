// RUN: %clang -O1 -arch arm64 -S -o - -emit-llvm %s | FileCheck %s

#include <aarch64_simd.h>

float64x2_t test_vcvt_f64_f32(float32x2_t x) {
  // CHECK: test_vcvt_f64_f32
  return vcvt_f64_f32(x);
  // CHECK: llvm.arm64.neon.vcvtfp2df
  // CHECK-NEXT: ret
}

float64x2_t test_vcvt_high_f64_f32(float32x4_t x) {
  // CHECK: test_vcvt_high_f64_f32
  return vcvt_high_f64_f32(x);
  // CHECK: llvm.arm64.neon.vcvthighfp2df
  // CHECK-NEXT: ret
}
