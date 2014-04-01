// RUN: %clang_cc1 -O1 -triple arm64-apple-ios7 -ffreestanding -S -o - -emit-llvm %s | FileCheck %s
// RUN: %clang_cc1 -O1 -triple arm64-apple-ios7 -ffreestanding -S -o - %s | \
// RUN:   FileCheck -check-prefix=CHECK_CODEGEN %s
// RUN: %clang_cc1 -O0 -triple arm64-apple-ios7 -ffreestanding -S -o - %s | \
// RUN:   FileCheck -check-prefix=CHECK_CODEGEN_O0 %s
// REQUIRES: arm64-registered-target

// Test ARM64 SIMD copy vector element to vector element: vcopyq_lane*

#include <aarch64_simd.h>

int8x16_t test_vcopyq_lane_s8(int8x16_t a1, int8x16_t a2) {
  // CHECK: test_vcopyq_lane_s8
  return vcopyq_lane_s8(a1, (int64_t) 3, a2, (int64_t) 13);
  // CHECK: llvm.arm64.neon.vcopy.lane.v16i8.v16i8
  // CHECK_CODEGEN: ins.b	v0[3], v1[13]
  // Do not reproduce the whole stack access for O0 as it may
  // change easily.
  // CHECK_CODEGEN_O0: test_vcopyq_lane_s8
  // CHECK_CODEGEN_O0: ins.b v{{[0-9]+}}[3], v{{[0-9]+}}[13]
}

uint8x16_t test_vcopyq_lane_u8(uint8x16_t a1, uint8x16_t a2) {
  // CHECK: test_vcopyq_lane_u8
  return vcopyq_lane_u8(a1, (int64_t) 3, a2, (int64_t) 13);
  // CHECK: llvm.arm64.neon.vcopy.lane.v16i8.v16i8
  // CHECK_CODEGEN: ins.b	v0[3], v1[13]
}

int16x8_t test_vcopyq_lane_s16(int16x8_t a1, int16x8_t a2) {
  // CHECK: test_vcopyq_lane_s16
  return vcopyq_lane_s16(a1, (int64_t) 3, a2, (int64_t) 7);
  // CHECK: llvm.arm64.neon.vcopy.lane.v8i16.v8i16
  // CHECK_CODEGEN: ins.h	v0[3], v1[7]
}

uint16x8_t test_vcopyq_lane_u16(uint16x8_t a1, uint16x8_t a2) {
  // CHECK: test_vcopyq_lane_u16
  return vcopyq_lane_u16(a1, (int64_t) 3, a2, (int64_t) 7);
  // CHECK: llvm.arm64.neon.vcopy.lane.v8i16.v8i16
  // CHECK_CODEGEN: ins.h	v0[3], v1[7]
}

int32x4_t test_vcopyq_lane_s32(int32x4_t a1, int32x4_t a2) {
  // CHECK: test_vcopyq_lane_s32
  return vcopyq_lane_s32(a1, (int64_t) 3, a2, (int64_t) 3);
  // CHECK: llvm.arm64.neon.vcopy.lane.v4i32.v4i32
  // CHECK_CODEGEN: ins.s	v0[3], v1[3]
}

uint32x4_t test_vcopyq_lane_u32(uint32x4_t a1, uint32x4_t a2) {
  // CHECK: test_vcopyq_lane_u32
  return vcopyq_lane_u32(a1, (int64_t) 3, a2, (int64_t) 3);
  // CHECK: llvm.arm64.neon.vcopy.lane.v4i32.v4i32
  // CHECK_CODEGEN: ins.s	v0[3], v1[3]
}

int64x2_t test_vcopyq_lane_s64(int64x2_t a1, int64x2_t a2) {
  // CHECK: test_vcopyq_lane_s64
  return vcopyq_lane_s64(a1, (int64_t) 0, a2, (int64_t) 1);
  // CHECK: llvm.arm64.neon.vcopy.lane.v2i64.v2i64
  // CHECK_CODEGEN: ins.d	v0[0], v1[1]
}

uint64x2_t test_vcopyq_lane_u64(uint64x2_t a1, uint64x2_t a2) {
  // CHECK: test_vcopyq_lane_u64
  return vcopyq_lane_u64(a1, (int64_t) 0, a2, (int64_t) 1);
  // CHECK: llvm.arm64.neon.vcopy.lane.v2i64.v2i64
  // CHECK_CODEGEN: ins.d	v0[0], v1[1]
}

float32x4_t test_vcopyq_lane_f32(float32x4_t a1, float32x4_t a2) {
  // CHECK: test_vcopyq_lane_f32
  return vcopyq_lane_f32(a1, 0, a2, 3);
  // CHECK: llvm.arm64.neon.vcopy.lane.v4i32.v4i32
  // CHECK_CODEGEN: ins.s	v0[0], v1[3]
}

float64x2_t test_vcopyq_lane_f64(float64x2_t a1, float64x2_t a2) {
  // CHECK: test_vcopyq_lane_f64
  return vcopyq_lane_f64(a1, 0, a2, 1);
  // CHECK: llvm.arm64.neon.vcopy.lane.v2i64.v2i64
  // CHECK_CODEGEN: ins.d	v0[0], v1[1]
}

