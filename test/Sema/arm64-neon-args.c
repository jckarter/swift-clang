// RUN: %clang_cc1 -triple arm64-apple-darwin -fsyntax-only -ffreestanding -verify %s

#include <aarch64_simd.h>

// rdar://13527900
void vcopy_reject(float32x4_t vOut0, float32x4_t vAlpha, int t) {
  vcopyq_lane_f32(vOut0, 1, vAlpha, t); // expected-error {{argument to '__builtin_arm64_vcopyq_lane_v' must be a constant integer}}
}

// rdar://problem/15256199
float32x4_t test_vmlsq_lane(float32x4_t accum, float32x4_t lhs, float32x2_t rhs) {
  return vmlsq_lane_f32(accum, lhs, rhs, 1);
}
