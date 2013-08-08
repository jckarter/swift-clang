// RUN: %clang -O1 -target arm64 -ffreestanding -S -o - -emit-llvm %s | FileCheck %s

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

float32x2_t test_vcvt_f32_f64(float64x2_t v) {
  // CHECK: test_vcvt_f32_f64
  return vcvt_f32_f64(v);
  // CHECK: llvm.arm64.neon.vcvtdf2fp
  // CHECK-NEXT: ret
}

float32x4_t test_vcvt_high_f32_f64(float32x2_t x, float64x2_t v) {
  // CHECK: test_vcvt_high_f32_f64
  return vcvt_high_f32_f64(x, v);
  // CHECK: llvm.arm64.neon.vcvthighdf2fp
  // CHECK-NEXT: ret
}

float32x2_t test_vcvtx_f32_f64(float64x2_t v) {
  // CHECK: test_vcvtx_f32_f64
  return vcvtx_f32_f64(v);
  // CHECK: llvm.arm64.neon.fcvtxn.v2f32.v2f64
  // CHECK-NEXT: ret
}

float32x4_t test_vcvtx_high_f32_f64(float32x2_t x, float64x2_t v) {
  // CHECK: test_vcvtx_high_f32_f64
  return vcvtx_high_f32_f64(x, v);
  // CHECK: llvm.arm64.neon.fcvtxn.v2f32.v2f64
  // CHECK: shufflevector
  // CHECK-NEXT: ret
}
