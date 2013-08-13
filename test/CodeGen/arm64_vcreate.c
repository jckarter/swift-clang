// RUN: %clang -O1 -target arm64-apple-ios7 -ffreestanding -S -o - -emit-llvm %s | FileCheck %s
// Test ARM64 SIMD vcreate intrinsics

/*#include <aarch64_simd.h>*/
#include <arm_neon.h>

float32x2_t test_vcreate_f32(uint64_t a1) {
  // CHECK: test_vcreate_f32
  return vcreate_f32(a1);
  // CHECK: bitcast {{.*}} to <2 x float>
  // CHECK-NEXT: ret
}

// FIXME enable when scalar_to_vector in backend is fixed.  Also, change
// CHECK@ to CHECK<colon> and CHECK-NEXT@ to CHECK-NEXT<colon>
/* 
float64x1_t test_vcreate_f64(uint64_t a1) {
  // CHECK@ test_vcreate_f64
  return vcreate_f64(a1);
  // CHECK@ llvm.arm64.neon.saddlv.i64.v2i32
  // CHECK-NEXT@ ret
}
*/
