// RUN: %clang -O1 -arch arm64 -S -o - -emit-llvm %s | FileCheck %s
// Test ARM64 SIMD load and stores of an N-element structure  intrinsics

#include <aarch64_simd.h>

int64x2x2_t test_vld2q_lane_s64(const void * a1, int64x2x2_t a2) {
  // CHECK: test_vld2q_lane_s64
  return vld2q_lane_s64(a1, a2, 1);
  // CHECK: llvm.arm64.neon.ld2lane.v2i64.p0i8
}

uint64x2x2_t test_vld2q_lane_u64(const void * a1, uint64x2x2_t a2) {
  // CHECK: test_vld2q_lane_u64
  return vld2q_lane_u64(a1, a2, 1);
  // CHECK: llvm.arm64.neon.ld2lane.v2i64.p0i8
}

int64x1x2_t test_vld2_lane_s64(const void * a1, int64x1x2_t a2) {
  // CHECK: test_vld2_lane_s64
  return vld2_lane_s64(a1, a2, 0);
  // CHECK: llvm.arm64.neon.ld2lane.v1i64.p0i8
}

uint64x1x2_t test_vld2_lane_u64(const void * a1, uint64x1x2_t a2) {
  // CHECK: test_vld2_lane_u64
  return vld2_lane_u64(a1, a2, 0);
  // CHECK: llvm.arm64.neon.ld2lane.v1i64.p0i8
}

int64x2x3_t test_vld3q_lane_s64(const void * a1, int64x2x3_t a2) {
  // CHECK: test_vld3q_lane_s64
  return vld3q_lane_s64(a1, a2, 1);
  // CHECK: llvm.arm64.neon.ld3lane.v2i64.p0i8
}

uint64x2x3_t test_vld3q_lane_u64(const void * a1, uint64x2x3_t a2) {
  // CHECK: test_vld3q_lane_u64
  return vld3q_lane_u64(a1, a2, 1);
  // CHECK: llvm.arm64.neon.ld3lane.v2i64.p0i8
}

int64x1x3_t test_vld3_lane_s64(const void * a1, int64x1x3_t a2) {
  // CHECK: test_vld3_lane_s64
  return vld3_lane_s64(a1, a2, 0);
  // CHECK: llvm.arm64.neon.ld3lane.v1i64.p0i8
}

uint64x1x3_t test_vld3_lane_u64(const void * a1, uint64x1x3_t a2) {
  // CHECK: test_vld3_lane_u64
  return vld3_lane_u64(a1, a2, 0);
  // CHECK: llvm.arm64.neon.ld3lane.v1i64.p0i8
}

int64x2x4_t test_vld4q_lane_s64(const void * a1, int64x2x4_t a2) {
  // CHECK: test_vld4q_lane_s64
  return vld4q_lane_s64(a1, a2, 0);
  // CHECK: llvm.arm64.neon.ld4lane.v2i64.p0i8
}

uint64x2x4_t test_vld4q_lane_u64(const void * a1, uint64x2x4_t a2) {
  // CHECK: test_vld4q_lane_u64
  return vld4q_lane_u64(a1, a2, 0);
  // CHECK: llvm.arm64.neon.ld4lane.v2i64.p0i8
}

int64x1x4_t test_vld4_lane_s64(const void * a1, int64x1x4_t a2) {
  // CHECK: test_vld4_lane_s64
  return vld4_lane_s64(a1, a2, 0);
  // CHECK: llvm.arm64.neon.ld4lane.v1i64.p0i8
}

uint64x1x4_t test_vld4_lane_u64(const void * a1, uint64x1x4_t a2) {
  // CHECK: test_vld4_lane_u64
  return vld4_lane_u64(a1, a2, 0);
  // CHECK: llvm.arm64.neon.ld4lane.v1i64.p0i8
}

