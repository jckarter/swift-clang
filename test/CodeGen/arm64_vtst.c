// RUN: %clang -O1 -target arm64 -ffreestanding -S -o - -emit-llvm %s | FileCheck %s
// Test ARM64 SIMD comparison test intrinsics

#include <aarch64_simd.h>

uint64x2_t test_vtstq_s64(int64x2_t a1, int64x2_t a2) {
  // CHECK: test_vtstq_s64
  return vtstq_s64(a1, a2);
  // CHECK: llvm.arm64.neon.cmtst.v2i64
  // CHECK-NEXT: ret
}

uint64x2_t test_vtstq_u64(uint64x2_t a1, uint64x2_t a2) {
  // CHECK: test_vtstq_u64
  return vtstq_u64(a1, a2);
  // CHECK: llvm.arm64.neon.cmtst.v2i64
  // CHECK-NEXT: ret
}
