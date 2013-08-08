// RUN: %clang_cc1 -triple arm64-apple-ios7.0 -ffreestanding -Os -S -o - %s | FileCheck %s
// REQUIRES: arm64-registered-target

#include <arm_neon.h>

int8x16_t test_vrshrn_high_n_s16(int8x8_t lowpart, int16x8_t input) {
  // CHECK: rshrn2.16b
  return vrshrn_high_n_s16(lowpart, input, 2);
}

int16x8_t test_vrshrn_high_n_s32(int16x4_t lowpart, int32x4_t input) {
  // CHECK: rshrn2.8h
  return vrshrn_high_n_s32(lowpart, input, 2);
}

int32x4_t test_vrshrn_high_n_s64(int32x2_t lowpart, int64x2_t input) {
  // CHECK: shrn2.4s
  return vrshrn_high_n_s64(lowpart, input, 2);
}

uint8x16_t test_vrshrn_high_n_u16(uint8x8_t lowpart, uint16x8_t input) {
  // CHECK: rshrn2.16b
  return vrshrn_high_n_u16(lowpart, input, 2);
}

uint16x8_t test_vrshrn_high_n_u32(uint16x4_t lowpart, uint32x4_t input) {
  // CHECK: rshrn2.8h
  return vrshrn_high_n_u32(lowpart, input, 2);
}

uint32x4_t test_vrshrn_high_n_u64(uint32x2_t lowpart, uint64x2_t input) {
  // CHECK: rshrn2.4s
  return vrshrn_high_n_u64(lowpart, input, 2);
}

int8x16_t test_vshrn_high_n_s16(int8x8_t lowpart, int16x8_t input) {
  // CHECK: shrn2.16b
  return vshrn_high_n_s16(lowpart, input, 2);
}

int16x8_t test_vshrn_high_n_s32(int16x4_t lowpart, int32x4_t input) {
  // CHECK: shrn2.8h
  return vshrn_high_n_s32(lowpart, input, 2);
}

int32x4_t test_vshrn_high_n_s64(int32x2_t lowpart, int64x2_t input) {
  // CHECK: shrn2.4s
  return vshrn_high_n_s64(lowpart, input, 2);
}

uint8x16_t test_vshrn_high_n_u16(uint8x8_t lowpart, uint16x8_t input) {
  // CHECK: shrn2.16b
  return vshrn_high_n_u16(lowpart, input, 2);
}

uint16x8_t test_vshrn_high_n_u32(uint16x4_t lowpart, uint32x4_t input) {
  // CHECK: shrn2.8h
  return vshrn_high_n_u32(lowpart, input, 2);
}

uint32x4_t test_vshrn_high_n_u64(uint32x2_t lowpart, uint64x2_t input) {
  // CHECK: shrn2.4s
  return vshrn_high_n_u64(lowpart, input, 2);
}

uint8x16_t test_vqshrun_high_n_s16(uint8x8_t lowpart, int16x8_t input) {
  // CHECK: sqshrun2.16b
  return vqshrun_high_n_s16(lowpart, input, 2);
}

uint16x8_t test_vqshrun_high_n_s32(uint16x4_t lowpart, int32x4_t input) {
  // CHECK: sqshrun2.8h
  return vqshrun_high_n_s32(lowpart, input, 2);
}

uint32x4_t test_vqshrun_high_n_s64(uint32x2_t lowpart, int64x2_t input) {
  // CHECK: sqshrun2.4s
  return vqshrun_high_n_s64(lowpart, input, 2);
}

uint8x16_t test_vqrshrun_high_n_s16(uint8x8_t lowpart, int16x8_t input) {
  // CHECK: sqrshrun2.16b
  return vqrshrun_high_n_s16(lowpart, input, 2);
}

uint16x8_t test_vqrshrun_high_n_s32(uint16x4_t lowpart, int32x4_t input) {
  // CHECK: sqrshrun2.8h
  return vqrshrun_high_n_s32(lowpart, input, 2);
}

uint32x4_t test_vqrshrun_high_n_s64(uint32x2_t lowpart, int64x2_t input) {
  // CHECK: sqrshrun2.4s
  return vqrshrun_high_n_s64(lowpart, input, 2);
}

int8x16_t test_vqshrn_high_n_s16(int8x8_t lowpart, int16x8_t input) {
  // CHECK: sqshrn2.16b
  return vqshrn_high_n_s16(lowpart, input, 2);
}

int16x8_t test_vqshrn_high_n_s32(int16x4_t lowpart, int32x4_t input) {
  // CHECK: sqshrn2.8h
  return vqshrn_high_n_s32(lowpart, input, 2);
}

int32x4_t test_vqshrn_high_n_s64(int32x2_t lowpart, int64x2_t input) {
  // CHECK: sqshrn2.4s
  return vqshrn_high_n_s64(lowpart, input, 2);
}

uint8x16_t test_vqshrn_high_n_u16(uint8x8_t lowpart, uint16x8_t input) {
  // CHECK: uqshrn2.16b
  return vqshrn_high_n_u16(lowpart, input, 2);
}

uint16x8_t test_vqshrn_high_n_u32(uint16x4_t lowpart, uint32x4_t input) {
  // CHECK: uqshrn2.8h
  return vqshrn_high_n_u32(lowpart, input, 2);
}

uint32x4_t test_vqshrn_high_n_u64(uint32x2_t lowpart, uint64x2_t input) {
  // CHECK: uqshrn2.4s
  return vqshrn_high_n_u64(lowpart, input, 2);
}

int8x16_t test_vqrshrn_high_n_s16(int8x8_t lowpart, int16x8_t input) {
  // CHECK: sqrshrn2.16b
  return vqrshrn_high_n_s16(lowpart, input, 2);
}

int16x8_t test_vqrshrn_high_n_s32(int16x4_t lowpart, int32x4_t input) {
  // CHECK: sqrshrn2.8h
  return vqrshrn_high_n_s32(lowpart, input, 2);
}

int32x4_t test_vqrshrn_high_n_s64(int32x2_t lowpart, int64x2_t input) {
  // CHECK: sqrshrn2.4s
  return vqrshrn_high_n_s64(lowpart, input, 2);
}

uint8x16_t test_vqrshrn_high_n_u16(uint8x8_t lowpart, uint16x8_t input) {
  // CHECK: uqrshrn2.16b
  return vqrshrn_high_n_u16(lowpart, input, 2);
}

uint16x8_t test_vqrshrn_high_n_u32(uint16x4_t lowpart, uint32x4_t input) {
  // CHECK: uqrshrn2.8h
  return vqrshrn_high_n_u32(lowpart, input, 2);
}

uint32x4_t test_vqrshrn_high_n_u64(uint32x2_t lowpart, uint64x2_t input) {
  // CHECK: uqrshrn2.4s
  return vqrshrn_high_n_u64(lowpart, input, 2);
}

int8x16_t test_vaddhn_high_s16(int8x8_t lowpart, int16x8_t lhs, int16x8_t rhs) {
  // CHECK: addhn2.16b v0, v1, v2
  return vaddhn_high_s16(lowpart, lhs, rhs);
}

int16x8_t test_vaddhn_high_s32(int16x4_t lowpart, int32x4_t lhs, int32x4_t rhs) {
  // CHECK: addhn2.8h v0, v1, v2
  return vaddhn_high_s32(lowpart, lhs, rhs);
}

int32x4_t test_vaddhn_high_s64(int32x2_t lowpart, int64x2_t lhs, int64x2_t rhs) {
  // CHECK: addhn2.4s v0, v1, v2
  return vaddhn_high_s64(lowpart, lhs, rhs);
}

uint8x16_t test_vaddhn_high_u16(uint8x8_t lowpart, uint16x8_t lhs, uint16x8_t rhs) {
  // CHECK: addhn2.16b v0, v1, v2
  return vaddhn_high_s16(lowpart, lhs, rhs);
}

uint16x8_t test_vaddhn_high_u32(uint16x4_t lowpart, uint32x4_t lhs, uint32x4_t rhs) {
  // CHECK: addhn2.8h v0, v1, v2
  return vaddhn_high_s32(lowpart, lhs, rhs);
}

uint32x4_t test_vaddhn_high_u64(uint32x2_t lowpart, uint64x2_t lhs, uint64x2_t rhs) {
  // CHECK: addhn2.4s v0, v1, v2
  return vaddhn_high_s64(lowpart, lhs, rhs);
}

int8x16_t test_vraddhn_high_s16(int8x8_t lowpart, int16x8_t lhs, int16x8_t rhs) {
  // CHECK: raddhn2.16b v0, v1, v2
  return vraddhn_high_s16(lowpart, lhs, rhs);
}

int16x8_t test_vraddhn_high_s32(int16x4_t lowpart, int32x4_t lhs, int32x4_t rhs) {
  // CHECK: raddhn2.8h v0, v1, v2
  return vraddhn_high_s32(lowpart, lhs, rhs);
}

int32x4_t test_vraddhn_high_s64(int32x2_t lowpart, int64x2_t lhs, int64x2_t rhs) {
  // CHECK: raddhn2.4s v0, v1, v2
  return vraddhn_high_s64(lowpart, lhs, rhs);
}

uint8x16_t test_vraddhn_high_u16(uint8x8_t lowpart, uint16x8_t lhs, uint16x8_t rhs) {
  // CHECK: raddhn2.16b v0, v1, v2
  return vraddhn_high_s16(lowpart, lhs, rhs);
}

uint16x8_t test_vraddhn_high_u32(uint16x4_t lowpart, uint32x4_t lhs, uint32x4_t rhs) {
  // CHECK: raddhn2.8h v0, v1, v2
  return vraddhn_high_s32(lowpart, lhs, rhs);
}

uint32x4_t test_vraddhn_high_u64(uint32x2_t lowpart, uint64x2_t lhs, uint64x2_t rhs) {
  // CHECK: raddhn2.4s v0, v1, v2
  return vraddhn_high_s64(lowpart, lhs, rhs);
}

int8x16_t test_vmovn_high_s16(int8x8_t lowpart, int16x8_t wide) {
  // CHECK: xtn2.16b v0, v1
  return vmovn_high_s16(lowpart, wide);
}

int16x8_t test_vmovn_high_s32(int16x4_t lowpart, int32x4_t wide) {
  // CHECK: xtn2.8h v0, v1
  return vmovn_high_s32(lowpart, wide);
}

int32x4_t test_vmovn_high_s64(int32x2_t lowpart, int64x2_t wide) {
  // CHECK: xtn2.4s v0, v1
  return vmovn_high_s64(lowpart, wide);
}

uint8x16_t test_vmovn_high_u16(uint8x8_t lowpart, uint16x8_t wide) {
  // CHECK: xtn2.16b v0, v1
  return vmovn_high_u16(lowpart, wide);
}

uint16x8_t test_vmovn_high_u32(uint16x4_t lowpart, uint32x4_t wide) {
  // CHECK: xtn2.8h v0, v1
  return vmovn_high_u32(lowpart, wide);
}

uint32x4_t test_vmovn_high_u64(uint32x2_t lowpart, uint64x2_t wide) {
  // CHECK: xtn2.4s v0, v1
  return vmovn_high_u64(lowpart, wide);
}

int8x16_t test_vqmovn_high_s16(int8x8_t lowpart, int16x8_t wide) {
  // CHECK: sqxtn2.16b v0, v1
  return vqmovn_high_s16(lowpart, wide);
}

int16x8_t test_vqmovn_high_s32(int16x4_t lowpart, int32x4_t wide) {
  // CHECK: sqxtn2.8h v0, v1
  return vqmovn_high_s32(lowpart, wide);
}

int32x4_t test_vqmovn_high_s64(int32x2_t lowpart, int64x2_t wide) {
  // CHECK: sqxtn2.4s v0, v1
  return vqmovn_high_s64(lowpart, wide);
}

uint8x16_t test_vqmovn_high_u16(uint8x8_t lowpart, int16x8_t wide) {
  // CHECK: uqxtn2.16b v0, v1
  return vqmovn_high_u16(lowpart, wide);
}

uint16x8_t test_vqmovn_high_u32(uint16x4_t lowpart, int32x4_t wide) {
  // CHECK: uqxtn2.8h v0, v1
  return vqmovn_high_u32(lowpart, wide);
}

uint32x4_t test_vqmovn_high_u64(uint32x2_t lowpart, int64x2_t wide) {
  // CHECK: uqxtn2.4s v0, v1
  return vqmovn_high_u64(lowpart, wide);
}

uint8x16_t test_vqmovun_high_s16(uint8x8_t lowpart, int16x8_t wide) {
  // CHECK: sqxtun2.16b v0, v1
  return vqmovun_high_s16(lowpart, wide);
}

uint16x8_t test_vqmovun_high_s32(uint16x4_t lowpart, int32x4_t wide) {
  // CHECK: sqxtun2.8h v0, v1
  return vqmovun_high_s32(lowpart, wide);
}

uint32x4_t test_vqmovun_high_s64(uint32x2_t lowpart, int64x2_t wide) {
  // CHECK: sqxtun2.4s v0, v1
  return vqmovun_high_s64(lowpart, wide);
}

float32x4_t test_vcvtx_high_f32_f64(float32x2_t lowpart, float64x2_t wide) {
  // CHECK: fcvtxn2 v0.4s, v1.2d
  return vcvtx_high_f32_f64(lowpart, wide);
}

float64x2_t test_vcvt_f64_f32(float32x2_t x) {
  // CHECK: fcvtl v0.2d, v0.2s
  return vcvt_f64_f32(x);
}

float64x2_t test_vcvt_high_f64_f32(float32x4_t x) {
  // CHECK: fcvtl2 v0.2d, v0.4s
  return vcvt_high_f64_f32(x);
}

float32x2_t test_vcvt_f32_f64(float64x2_t v) {
  // CHECK: fcvtn v0.2s, v0.2d
  return vcvt_f32_f64(v);
}

float32x4_t test_vcvt_high_f32_f64(float32x2_t x, float64x2_t v) {
  // CHECK: fcvtn2 v0.4s, v1.2d
  return vcvt_high_f32_f64(x, v);
}

float32x2_t test_vcvtx_f32_f64(float64x2_t v) {
  // CHECK: fcvtxn v0.2s, v0.2d
  return vcvtx_f32_f64(v);
}
