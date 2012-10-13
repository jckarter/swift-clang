// RUN: %clang -arch arm64 -S -o - -emit-llvm %s | FileCheck %s
// Test ARM64 SIMD duplicate lane intrinsic

#include <aarch64_simd.h>

void test_vdup_lane_s64(int64x1_t a1) {
  // CHECK: test_vdup_lane_s64
  vdup_lane_s64(a1, 0);
  // CHECK: shufflevector
}

void test_vdup_lane_u64(uint64x1_t a1) {
  // CHECK: test_vdup_lane_u64
  vdup_lane_u64(a1, 0);
  // CHECK: shufflevector
}
