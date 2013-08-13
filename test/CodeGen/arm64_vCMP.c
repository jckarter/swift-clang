// RUN: %clang -O1 -target arm64-apple-ios7 -ffreestanding -S -o - -emit-llvm %s | FileCheck %s

// Test ARM64 SIMD fused multiply add intrinsics

#include <aarch64_simd.h>

int64x2_t test_vabsq_s64(int64x2_t a1) {
  // CHECK: test_vabsq_s64
  return vabsq_s64(a1);
  // CHECK: llvm.arm64.neon.abs.v2i64
  // CHECK-NEXT: ret
}

int64_t test_vceqd_s64(int64_t a1, int64_t a2) {
  // CHECK: test_vceqd_s64
  return vceqd_s64(a1, a2);
  // CHECK: icmp eq i64 %a1, %a2
}

uint64x2_t test_vceqq_u64(uint64x2_t a1, uint64x2_t a2) {
  // CHECK: test_vceqq_u64
  return vceqq_u64(a1, a2);
  // CHECK:  icmp eq <2 x i64> %a1, %a2
}

uint64x2_t test_vcgeq_s64(int64x2_t a1, int64x2_t a2) {
  // CHECK: test_vcgeq_s64
  return vcgeq_s64(a1, a2);
  // CHECK:  icmp sge <2 x i64> %a1, %a2
}

uint64x2_t test_vcgeq_u64(uint64x2_t a1, uint64x2_t a2) {
  // CHECK: test_vcgeq_u64
  return vcgeq_u64(a1, a2);
  // CHECK:  icmp uge <2 x i64> %a1, %a2
}

uint64x2_t test_vcgtq_s64(int64x2_t a1, int64x2_t a2) {
  // CHECK: test_vcgtq_s64
  return vcgtq_s64(a1, a2);
  // CHECK: icmp sgt <2 x i64> %a1, %a2
}

uint64x2_t test_vcgtq_u64(uint64x2_t a1, uint64x2_t a2) {
  // CHECK: test_vcgtq_u64
  return vcgtq_u64(a1, a2);
  // CHECK: icmp ugt <2 x i64> %a1, %a2
}

uint64x2_t test_vcleq_s64(int64x2_t a1, int64x2_t a2) {
  // CHECK: test_vcleq_s64
  return vcleq_s64(a1, a2);
  // CHECK: icmp sle <2 x i64> %a1, %a2
}

uint64x2_t test_vcleq_u64(uint64x2_t a1, uint64x2_t a2) {
  // CHECK: test_vcleq_u64
  return vcleq_u64(a1, a2);
  // CHECK: icmp ule <2 x i64> %a1, %a2
}

uint64x2_t test_vcltq_s64(int64x2_t a1, int64x2_t a2) {
  // CHECK: test_vcltq_s64
  return vcltq_s64(a1, a2);
  // CHECK: icmp slt <2 x i64> %a1, %a2
}

uint64x2_t test_vcltq_u64(uint64x2_t a1, uint64x2_t a2) {
  // CHECK: test_vcltq_u64
  return vcltq_u64(a1, a2);
  // CHECK: icmp ult <2 x i64> %a1, %a2
}

int64x2_t test_vqabsq_s64(int64x2_t a1) {
  // CHECK: test_vqabsq_s64
  return vqabsq_s64(a1);
  // CHECK: llvm.arm64.neon.sqabs.v2i64(<2 x i64> %a1)
  // CHECK-NEXT: ret
}
