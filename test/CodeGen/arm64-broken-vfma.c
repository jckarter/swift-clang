// RUN: %clang_cc1 -triple arm64-apple-ios7.0 -target-feature +neon -ffreestanding -emit-llvm -o - -O1 %s | FileCheck %s
// REQUIRES: aarch64-registered-target

#define USE_CORRECT_VFMA_INTRINSICS 0
#include <arm_neon.h>

float32x2_t test_vfma_f32(float32x2_t acc, float32x2_t lhs, float32x2_t rhs) {
  // CHECK-LABEL: @test_vfma_f32
  // CHECK: [[LANE:%.*]] = shufflevector <2 x float> %lhs, <2 x float> undef, <2 x i32> <i32 1, i32 1>
  // CHECK: [[FMA:%.*]] = tail call <2 x float> @llvm.fma.v2f32(<2 x float> %rhs, <2 x float> [[LANE]], <2 x float> %acc)
  // CHECK: ret <2 x float> [[FMA]]
  return vfma_lane_f32(acc, lhs, rhs, 1);
}

float32x2_t test_vfms_f32(float32x2_t acc, float32x2_t lhs, float32x2_t rhs) {
  // CHECK-LABEL: @test_vfms_f32
  // CHECK: [[NEG:%.*]] = fsub <2 x float> <float -0.000000e+00, float -0.000000e+00>, %rhs
  // CHECK: [[LANE:%.*]] = shufflevector <2 x float> %lhs, <2 x float> undef, <2 x i32> <i32 1, i32 1>
  // CHECK: [[FMS:%.*]] = tail call <2 x float> @llvm.fma.v2f32(<2 x float> [[NEG]], <2 x float> [[LANE]], <2 x float> %acc)
  // CHECK: ret <2 x float> [[FMS]]
  return vfms_lane_f32(acc, lhs, rhs, 1);
}

float32x2_t test_vfmaq_f32(float32x2_t acc, float32x4_t lhs, float32x2_t rhs) {
  // CHECK-LABEL: @test_vfmaq_f32
  // CHECK: [[LANE:%.*]] = shufflevector <4 x float> %lhs, <4 x float> undef, <2 x i32> <i32 1, i32 1>
  // CHECK: [[FMA:%.*]] = tail call <2 x float> @llvm.fma.v2f32(<2 x float> [[LANE]], <2 x float> %rhs, <2 x float> %acc)
  // CHECK: ret <2 x float> [[FMA]]
  return vfma_laneq_f32(acc, lhs, rhs, 1);
}

float32x2_t test_vfmsq_f32(float32x2_t acc, float32x4_t lhs, float32x2_t rhs) {
  // CHECK-LABEL: @test_vfmsq_f32
  // CHECK: [[NEG:%.*]] = fsub <2 x float> <float -0.000000e+00, float -0.000000e+00>, %rhs
  // CHECK: [[LANE:%.*]] = shufflevector <4 x float> %lhs, <4 x float> undef, <2 x i32> <i32 1, i32 1>
  // CHECK: [[FMS:%.*]] = tail call <2 x float> @llvm.fma.v2f32(<2 x float> [[LANE]], <2 x float> [[NEG]], <2 x float> %acc)
  // CHECK: ret <2 x float> [[FMS]]
  return vfms_laneq_f32(acc, lhs, rhs, 1);
}


float64x2_t test_vfmaq_f64(float64x2_t acc, float64x2_t lhs, float64x2_t rhs) {
  // CHECK-LABEL: @test_vfmaq_f64
  // CHECK: [[LANE:%.*]] = shufflevector <2 x double> %lhs, <2 x double> undef, <2 x i32> <i32 1, i32 1>
  // CHECK: [[FMA:%.*]] = tail call <2 x double> @llvm.fma.v2f64(<2 x double> [[LANE]], <2 x double> %rhs, <2 x double> %acc)
  // CHECK: ret <2 x double> [[FMA]]
  return vfmaq_laneq_f64(acc, lhs, rhs, 1);
}

float64x2_t test_vfmsq_f64(float64x2_t acc, float64x2_t lhs, float64x2_t rhs) {
  // CHECK-LABEL: @test_vfmsq_f64
  // CHECK: [[NEG:%.*]] = fsub <2 x double> <double -0.000000e+00, double -0.000000e+00>, %rhs
  // CHECK: [[LANE:%.*]] = shufflevector <2 x double> %lhs, <2 x double> undef, <2 x i32> <i32 1, i32 1>
  // CHECK: [[FMS:%.*]] = tail call <2 x double> @llvm.fma.v2f64(<2 x double> [[LANE]], <2 x double> [[NEG]], <2 x double> %acc)
  // CHECK: ret <2 x double> [[FMS]]
  return vfmsq_laneq_f64(acc, lhs, rhs, 1);
}
