// REQUIRES: arm64-registered-target
// RUN: %clang_cc1 -triple arm64-apple-ios7.0  \
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

// CHECK: test_vqrshlb_s8
int8_t test_vqrshlb_s8(int8_t a, int8_t b) {
  return vqrshlb_s8(a, b);
// CHECK: sqrshl.8b {{v[0-9]+}}, {{v[0-9]+}}, {{v[0-9]+}}
}

// CHECK: test_vqrshlh_s16
int16_t test_vqrshlh_s16(int16_t a, int16_t b) {
  return vqrshlh_s16(a, b);
// CHECK: sqrshl.4h {{v[0-9]+}}, {{v[0-9]+}}, {{v[0-9]+}}
}

// CHECK: test_vqrshls_s32
int32_t test_vqrshls_s32(int32_t a, int32_t b) {
  return vqrshls_s32(a, b);
// CHECK: sqrshl {{s[0-9]+}}, {{s[0-9]+}}, {{s[0-9]+}}
}

// CHECK: test_vqrshld_s64
int64_t test_vqrshld_s64(int64_t a, int64_t b) {
  return vqrshld_s64(a, b);
// CHECK: sqrshl {{d[0-9]+}}, {{d[0-9]+}}, {{d[0-9]+}}
}

// CHECK: test_vqrshlb_u8
uint8_t test_vqrshlb_u8(uint8_t a, uint8_t b) {
  return vqrshlb_u8(a, b);
// CHECK: uqrshl.8b {{v[0-9]+}}, {{v[0-9]+}}, {{v[0-9]+}}
}

// CHECK: test_vqrshlh_u16
uint16_t test_vqrshlh_u16(uint16_t a, uint16_t b) {
  return vqrshlh_u16(a, b);
// CHECK: uqrshl.4h {{v[0-9]+}}, {{v[0-9]+}}, {{v[0-9]+}}
}

// CHECK: test_vqrshls_u32
uint32_t test_vqrshls_u32(uint32_t a, uint32_t b) {
  return vqrshls_u32(a, b);
// CHECK: uqrshl {{s[0-9]+}}, {{s[0-9]+}}, {{s[0-9]+}}
}

// CHECK: test_vqrshld_u64
uint64_t test_vqrshld_u64(uint64_t a, uint64_t b) {
  return vqrshld_u64(a, b);
// CHECK: uqrshl {{d[0-9]+}}, {{d[0-9]+}}, {{d[0-9]+}}
}

// CHECK: test_vqshlb_s8
int8_t test_vqshlb_s8(int8_t a, int8_t b) {
  return vqshlb_s8(a, b);
// CHECK: sqshl.8b {{v[0-9]+}}, {{v[0-9]+}}, {{v[0-9]+}}
}

// CHECK: test_vqshlh_s16
int16_t test_vqshlh_s16(int16_t a, int16_t b) {
  return vqshlh_s16(a, b);
// CHECK: sqshl.4h {{v[0-9]+}}, {{v[0-9]+}}, {{v[0-9]+}}
}

// CHECK: test_vqshls_s32
int32_t test_vqshls_s32(int32_t a, int32_t b) {
  return vqshls_s32(a, b);
// CHECK: sqshl {{s[0-9]+}}, {{s[0-9]+}}, {{s[0-9]+}}
}

// CHECK: test_vqshld_s64
int64_t test_vqshld_s64(int64_t a, int64_t b) {
  return vqshld_s64(a, b);
// CHECK: sqshl {{d[0-9]+}}, {{d[0-9]+}}, {{d[0-9]+}}
}

// CHECK: test_vqshlb_u8
uint8_t test_vqshlb_u8(uint8_t a, uint8_t b) {
  return vqshlb_u8(a, b);
// CHECK: uqshl.8b {{v[0-9]+}}, {{v[0-9]+}}, {{v[0-9]+}}
}

// CHECK: test_vqshlh_u16
uint16_t test_vqshlh_u16(uint16_t a, uint16_t b) {
  return vqshlh_u16(a, b);
// CHECK: uqshl.4h {{v[0-9]+}}, {{v[0-9]+}}, {{v[0-9]+}}
}

// CHECK: test_vqshls_u32
uint32_t test_vqshls_u32(uint32_t a, uint32_t b) {
  return vqshls_u32(a, b);
// CHECK: uqshl {{s[0-9]+}}, {{s[0-9]+}}, {{s[0-9]+}}
}

// CHECK: test_vqshld_u64
uint64_t test_vqshld_u64(uint64_t a, uint64_t b) {
  return vqshld_u64(a, b);
// CHECK: uqshl {{d[0-9]+}}, {{d[0-9]+}}, {{d[0-9]+}}
}

// CHECK: test_vshld_u64
uint64_t test_vshld_u64(uint64_t a, uint64_t b) {
  return vshld_u64(a, b);
// CHECK: ushl {{d[0-9]+}}, {{d[0-9]+}}, {{d[0-9]+}}
}

// CHECK: test_vshld_s64
int64_t test_vshld_s64(int64_t a, int64_t b) {
  return vshld_s64(a, b);
// CHECK: sshl {{d[0-9]+}}, {{d[0-9]+}}, {{d[0-9]+}}
}

// CHECK: test_vqdmullh_s16
int32_t test_vqdmullh_s16(int16_t a, int16_t b) {
  return vqdmullh_s16(a, b);
// CHECK: sqdmull.4s {{v[0-9]+}}, {{v[0-9]+}}, {{v[0-9]+}}
}

// CHECK: test_vqdmulls_s32
int64_t test_vqdmulls_s32(int32_t a, int32_t b) {
  return vqdmulls_s32(a, b);
// CHECK: sqdmull {{d[0-9]+}}, {{s[0-9]+}}, {{s[0-9]+}}
}
