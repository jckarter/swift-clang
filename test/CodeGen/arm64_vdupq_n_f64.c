// RUN: %clang -O3 -arch arm64 -c -S -o - %s | FileCheck %s
// REQUIRES: arm64-registered-target
/// Test vdupq_n_f64 and vmovq_nf64 ARM64 intrinsics
// <rdar://problem/11778405> ARM64: vdupq_n_f64 and vdupq_lane_f64 intrinsics 
// missing


#include <aarch64_simd.h>

// vdupq_n_f64 -> dup.2d v0, v0[0]
// 
float64x2_t test_vdupq_n_f64(float64_t w)
{
    return vdupq_n_f64(w);
  // CHECK: test_vdupq_n_f64:
  // CHECK: dup.2d v0, v0[0]
  // CHECK-NEXT: ret
}

// might as well test this while we're here
// vdupq_n_f32 -> dup.4s v0, v0[0]
float32x4_t test_vdupq_n_f32(float32_t w)
{
    return vdupq_n_f32(w);
  // CHECK: test_vdupq_n_f32:
  // CHECK: dup.4s v0, v0[0]
  // CHECK-NEXT: ret
}

// vdupq_lane_f64 -> dup.2d v0, v0[0]
// this was in <rdar://problem/11778405>, but had already been implemented, 
// test anyway
float64x2_t test_vdupq_lane_f64(float64x1_t V)
{
    return vdupq_lane_f64(V, 0);
  // CHECK: test_vdupq_lane_f64:
  // CHECK: dup.2d v0, v0[0]
  // CHECK-NEXT: ret
}

// vmovq_n_f64 -> dup Vd.2d,X0
// this wasn't in <rdar://problem/11778405>, but it was between the vdups
float64x2_t test_vmovq_n_f64(float64_t w)
{
  return vmovq_n_f64(w);
  // CHECK: test_vmovq_n_f64:
  // CHECK: dup.2d v0, v0[0]
  // CHECK-NEXT: ret
}
