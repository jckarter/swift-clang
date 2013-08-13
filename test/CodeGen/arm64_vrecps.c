// RUN: %clang -O3 -target arm64-apple-ios7 -ffreestanding -c -S -o - %s | FileCheck %s
/// Test vrecpss_f32, vrecpsd_f64 ARM64 intrinsics


#include <aarch64_simd.h>

// vrecpss_f32 -> FRECPS Sd,Sa,Sb
// 
float32_t test_vrecpss_f32(float32_t Vdlow, float32_t Vn)
{
    return vrecpss_f32(Vdlow, Vn);
  // CHECK: test_vrecpss_f32:
  // CHECK: frecps  s0, s0, s1
  // CHECK-NEXT: ret
}

// vrecpsd_f64 -> FRECPS Dd,Da,Db
// 
float64_t test_vrecpsd_f64(float64_t Vdlow, float64_t Vn)
{
    return vrecpsd_f64(Vdlow, Vn);
  // CHECK: test_vrecpsd_f64:
  // CHECK: frecps d0, d0, d1
  // CHECK-NEXT: ret
}
