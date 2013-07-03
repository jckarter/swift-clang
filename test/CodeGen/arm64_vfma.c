// RUN: %clang -O1 -target arm64 -ffreestanding -S -o - -emit-llvm %s | FileCheck %s
// Test ARM64 SIMD fused multiply add intrinsics

#include <aarch64_simd.h>

float32x2_t test_vfma_f32(float32x2_t a1, float32x2_t a2, float32x2_t a3) {
  // CHECK: test_vfma_f32
  return vfma_f32(a1, a2, a3);
  // CHECK: llvm.fma.v2f32({{.*a3, .*a2, .*a1}})
  // CHECK-NEXT: ret
}

float32x4_t test_vfmaq_f32(float32x4_t a1, float32x4_t a2, float32x4_t a3) {
  // CHECK: test_vfmaq_f32
  return vfmaq_f32(a1, a2, a3);
  // CHECK: llvm.fma.v4f32({{.*a3, .*a2, .*a1}})
  // CHECK-NEXT: ret
}

float64x2_t test_vfmaq_f64(float64x2_t a1, float64x2_t a2, float64x2_t a3) {
  // CHECK: test_vfmaq_f64
  return vfmaq_f64(a1, a2, a3);
  // CHECK: llvm.fma.v2f64({{.*a3, .*a2, .*a1}})
  // CHECK-NEXT: ret
}

float32x2_t test_vfma_lane_f32(float32x2_t a1, float32x2_t a2, float32x2_t a3) {
  // CHECK: test_vfma_lane_f32
  return vfma_lane_f32(a1, a2, a3, 1);
  // NB: the test below is deliberately lose, so that we don't depend too much
  // upon the exact IR used to select lane 1 (usually a shufflevector)
  // CHECK: llvm.fma.v2f32
  // CHECK-NEXT: ret
}

float32x4_t test_vfmaq_lane_f32(float32x4_t a1, float32x4_t a2, float32x4_t a3) {
  // CHECK: test_vfmaq_lane_f32
  return vfmaq_lane_f32(a1, a2, a3, 1);
  // NB: the test below is deliberately lose, so that we don't depend too much
  // upon the exact IR used to select lane 1 (usually a shufflevector)
  // CHECK: llvm.fma.v4f32
  // CHECK-NEXT: ret
}

float64x2_t test_vfmaq_lane_f64(float64x2_t a1, float64x2_t a2, float64x2_t a3) {
  // CHECK: test_vfmaq_lane_f64
  return vfmaq_lane_f64(a1, a2, a3, 1);
  // NB: the test below is deliberately lose, so that we don't depend too much
  // upon the exact IR used to select lane 1 (usually a shufflevector)
  // CHECK: llvm.fma.v2f64
  // CHECK-NEXT: ret
}

float32x2_t test_vfma_n_f32(float32x2_t a1, float32x2_t a2, float32_t a3) {
  // CHECK: test_vfma_n_f32
  return vfma_n_f32(a1, a2, a3);
  // NB: the test below is deliberately lose, so that we don't depend too much
  // upon the exact IR used to select lane 0 (usually two insertelements)
  // CHECK: llvm.fma.v2f32
  // CHECK-NEXT: ret
}

float32x4_t test_vfmaq_n_f32(float32x4_t a1, float32x4_t a2, float32_t a3) {
  // CHECK: test_vfmaq_n_f32
  return vfmaq_n_f32(a1, a2, a3);
  // NB: the test below is deliberately lose, so that we don't depend too much
  // upon the exact IR used to select lane 0 (usually four insertelements)
  // CHECK: llvm.fma.v4f32
  // CHECK-NEXT: ret
}

float64x2_t test_vfmaq_n_f64(float64x2_t a1, float64x2_t a2, float64_t a3) {
  // CHECK: test_vfmaq_n_f64
  return vfmaq_n_f64(a1, a2, a3);
  // NB: the test below is deliberately lose, so that we don't depend too much
  // upon the exact IR used to select lane 0 (usually two insertelements)
  // CHECK: llvm.fma.v2f64
  // CHECK-NEXT: ret
}
