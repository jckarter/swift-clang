// RUN: %clang_cc1 -triple arm64-apple-ios7.0 -target-feature +neon -ffreestanding -DUSE_CORRECT_VFMA_INTRINSICS=0 -fsyntax-only -verify %s
#include <arm_neon.h>

// expected-no-diagnostics

void foo(float32x2_t v2f32, float32x4_t v4f32,
         float64x1_t v1f64, float64x2_t v2f64) {
  vfma_lane_f32(v2f32, v2f32, v2f32, 0);
  vfma_lane_f64(v1f64, v1f64, v1f64, 0);

  vfms_lane_f32(v2f32, v2f32, v2f32, 0);
  vfms_lane_f64(v1f64, v1f64, v1f64, 0);
}
