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
