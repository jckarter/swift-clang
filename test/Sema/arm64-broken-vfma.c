// RUN: %clang_cc1 -triple arm64-apple-ios7.0 -target-feature +neon -ffreestanding -fsyntax-only -verify %s
#include <arm_neon.h>

void foo(float32x2_t v2f32, float32x4_t v4f32,
         float64x1_t v1f64, float64x2_t v2f64) {
  vfma_lane_f32(v2f32, v2f32, v2f32, 0); // expected-warning {{previous versions of Clang incorrectly interpreted vfmX_lane(a,b,c,i) as a + b[i]*c. Future releases of Clang will fix this unconditionally. Current behaviour is controlled by the USE_CORRECT_VFMA_INTRINSICS macro, which should be defined before including arm_neon.h. Leaving this undefined will retain the old behaviour with this warning; setting it to 0 will silence this warning but retain the old behaviour; setting it to 1 will emit compliant code.}}

  // No diagnostic needed here: there is only one lane so the wrong behaviour is
  // equivalent to the correct behaviour.
  vfma_lane_f64(v1f64, v1f64, v1f64, 0);

  vfms_lane_f32(v2f32, v2f32, v2f32, 0); // expected-warning {{previous versions of Clang incorrectly interpreted vfmX_lane(a,b,c,i) as a + b[i]*c. Future releases of Clang will fix this unconditionally. Current behaviour is controlled by the USE_CORRECT_VFMA_INTRINSICS macro, which should be defined before including arm_neon.h. Leaving this undefined will retain the old behaviour with this warning; setting it to 0 will silence this warning but retain the old behaviour; setting it to 1 will emit compliant code.}}

  // No diagnostic needed here: there is only one lane so the wrong behaviour is
  // equivalent to the correct behaviour.
  vfms_lane_f64(v1f64, v1f64, v1f64, 0);
}

void fooq(float32x2_t v2f32, float32x4_t v4f32,
          float64x1_t v1f64, float64x2_t v2f64) {
  vfma_laneq_f32(v2f32, v4f32, v2f32, 0); // expected-warning {{previous versions of Clang incorrectly interpreted vfmX_lane(a,b,c,i) as a + b[i]*c. Future releases of Clang will fix this unconditionally. Current behaviour is controlled by the USE_CORRECT_VFMA_INTRINSICS macro, which should be defined before including arm_neon.h. Leaving this undefined will retain the old behaviour with this warning; setting it to 0 will silence this warning but retain the old behaviour; setting it to 1 will emit compliant code.}}

  vfma_laneq_f64(v1f64, v2f64, v1f64, 0); // expected-warning {{previous versions of Clang incorrectly interpreted vfmX_lane(a,b,c,i) as a + b[i]*c. Future releases of Clang will fix this unconditionally. Current behaviour is controlled by the USE_CORRECT_VFMA_INTRINSICS macro, which should be defined before including arm_neon.h. Leaving this undefined will retain the old behaviour with this warning; setting it to 0 will silence this warning but retain the old behaviour; setting it to 1 will emit compliant code.}}

  vfms_lane_f32(v2f32, v2f32, v2f32, 0); // expected-warning {{previous versions of Clang incorrectly interpreted vfmX_lane(a,b,c,i) as a + b[i]*c. Future releases of Clang will fix this unconditionally. Current behaviour is controlled by the USE_CORRECT_VFMA_INTRINSICS macro, which should be defined before including arm_neon.h. Leaving this undefined will retain the old behaviour with this warning; setting it to 0 will silence this warning but retain the old behaviour; setting it to 1 will emit compliant code.}}

  vfms_laneq_f64(v1f64, v2f64, v1f64, 0); // expected-warning {{previous versions of Clang incorrectly interpreted vfmX_lane(a,b,c,i) as a + b[i]*c. Future releases of Clang will fix this unconditionally. Current behaviour is controlled by the USE_CORRECT_VFMA_INTRINSICS macro, which should be defined before including arm_neon.h. Leaving this undefined will retain the old behaviour with this warning; setting it to 0 will silence this warning but retain the old behaviour; setting it to 1 will emit compliant code.}}
}
