// RUN: %clang -O1 -arch arm64 -S -o - -emit-llvm %s | FileCheck %s
// what exists now: vcale_f32 vcaleq_f32 vcalt_f32 vcaltq_f32  

#include <aarch64_simd.h>

uint32x2_t test_vcale_f32(float32x2_t a1, float32x2_t a2) {
  // CHECK: test_vcale_f32
  return vcale_f32(a1, a2);
  // CHECK: llvm.arm64.neon.facge.v2f32
  // no check for ret here, as there is a bitcast
}

uint32x4_t test_vcaleq_f32(float32x4_t a1, float32x4_t a2) {
  // CHECK: test_vcaleq_f32
  return vcaleq_f32(a1, a2);
  // CHECK: llvm.arm64.neon.facge.v4f32{{.*a2,.*a1}}
  // no check for ret here, as there is a bitcast
}

uint32x2_t test_vcalt_f32(float32x2_t a1, float32x2_t a2) {
  // CHECK: test_vcalt_f32
  return vcalt_f32(a1, a2);
  // CHECK: llvm.arm64.neon.facgt.v2f32{{.*a2,.*a1}}
  // no check for ret here, as there is a bitcast
}

uint32x4_t test_vcaltq_f32(float32x4_t a1, float32x4_t a2) {
  // CHECK: test_vcaltq_f32
  return vcaltq_f32(a1, a2);
  // CHECK: llvm.arm64.neon.facgt.v4f32{{.*a2,.*a1}}
  // no check for ret here, as there is a bitcast
}
