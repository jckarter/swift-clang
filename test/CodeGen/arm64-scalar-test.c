// REQUIRES: arm64-registered-target
// RUN: %clang_cc1 -triple arm64-none-linux-gnu  \
// RUN:   -S -O1 -o - -ffreestanding %s | FileCheck %s

// We're explicitly using aarch64_simd.h here: some types probably don't match
// the ACLE definitions, but we want to check current codegen.
#include <aarch64_simd.h>

float test_vrsqrtss_f32(float a, float b) {
// CHECK: test_vrsqrtss_f32
  return vrsqrtss_f32(a, b);
// CHECK: frsqrts {{s[0-9]+}}, {{s[0-9]+}}, {{s[0-9]+}}
}

double test_vrsqrtsd_f64(double a, double b) {
// CHECK: test_vrsqrtsd_f64
  return vrsqrtsd_f64(a, b);
// CHECK: frsqrts {{d[0-9]+}}, {{d[0-9]+}}, {{d[0-9]+}}
}

int64_t test_vrshl_s64(int64_t a, int64_t b) {
// CHECK: test_vrshl_s64
  return vrshl_s64(a, b);
// CHECK: srshl {{d[0-9]+}}, {{d[0-9]+}}, {{d[0-9]+}}
}

uint64_t test_vrshl_u64(uint64_t a, int64_t b) {
// CHECK: test_vrshl_u64
  return vrshl_u64(a, b);
// CHECK: urshl {{d[0-9]+}}, {{d[0-9]+}}, {{d[0-9]+}}
}

// CHECK: test_vrshld_s64
int64_t test_vrshld_s64(int64_t a, int64_t b) {
  return vrshld_s64(a, b);
// CHECK: srshl {{d[0-9]+}}, {{d[0-9]+}}, {{d[0-9]+}}
}


// CHECK: test_vrshld_u64
uint64_t test_vrshld_u64(uint64_t a, uint64_t b) {
  return vrshld_u64(a, b);
// CHECK: urshl {{d[0-9]+}}, {{d[0-9]+}}, {{d[0-9]+}}
}
