// RUN: %clang_cc1 -triple thumbv7k-apple-ios7.0 -target-abi apcs-vfp %s -o - -emit-llvm | FileCheck %s

typedef struct {
  float arr[4];
} HFA;

// CHECK: define void @simple_hfa([4 x float] %h.coerce)
void simple_hfa(HFA h) {}

typedef struct {
  double arr[4];
} BigHFA;

// We don't want any padding type to be included by Clang when using the
// APCS-VFP ABI, that needs to be handled by LLVM if needed.

// CHECK: void @no_padding(i32 %r0, i32 %r1, i32 %r2, [4 x double] %d0_d3.coerce, [4 x double] %d4_d7.coerce, [4 x double] %sp.coerce, i64 %split)
void no_padding(int r0, int r1, int r2, BigHFA d0_d3, BigHFA d4_d7, BigHFA sp,
                long long split) {}
