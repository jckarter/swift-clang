// RUN: %clang_cc1 -triple arm64-apple-ios -ffreestanding -emit-llvm -w -o - %s | FileCheck %s

// CHECK: define signext i8 @f0()
char f0(void) {
  return 0;
}

// Struct as return type. Aggregates <= 16 bytes are passed directly and round
// up to multiple of 8 bytes.
// CHECK: define i64 @f1()
struct s1 { char f0; };
struct s1 f1(void) {}

// CHECK: define i64 @f2()
struct s2 { short f0; };
struct s2 f2(void) {}

// CHECK: define i64 @f3()
struct s3 { int f0; };
struct s3 f3(void) {}

// CHECK: define i64 @f4()
struct s4 { struct s4_0 { int f0; } f0; };
struct s4 f4(void) {}

// CHECK: define i64 @f5()
struct s5 { struct { } f0; int f1; };
struct s5 f5(void) {}

// CHECK: define i64 @f6()
struct s6 { int f0[1]; };
struct s6 f6(void) {}

// CHECK: define void @f7()
struct s7 { struct { int : 0; } f0; };
struct s7 f7(void) {}

// CHECK: define void @f8()
struct s8 { struct { int : 0; } f0[1]; };
struct s8 f8(void) {}

// CHECK: define i64 @f9()
struct s9 { int f0; int : 0; };
struct s9 f9(void) {}

// CHECK: define i64 @f10()
struct s10 { int f0; int : 0; int : 0; };
struct s10 f10(void) {}

// CHECK: define i64 @f11()
struct s11 { int : 0; int f0; };
struct s11 f11(void) {}

// CHECK: define i64 @f12()
union u12 { char f0; short f1; int f2; };
union u12 f12(void) {}

// Homogeneous Aggregate as return type will be passed directly.
// CHECK: define %struct.s13 @f13()
struct s13 { float f0; };
struct s13 f13(void) {}
// CHECK: define %union.u14 @f14()
union u14 { float f0; };
union u14 f14(void) {}

// CHECK: define void @f15()
void f15(struct s7 a0) {}

// CHECK: define void @f16()
void f16(struct s8 a0) {}

// CHECK: define i64 @f17()
struct s17 { short f0 : 13; char f1 : 4; };
struct s17 f17(void) {}

// CHECK: define i64 @f18()
struct s18 { short f0; char f1 : 4; };
struct s18 f18(void) {}

// CHECK: define i64 @f19()
struct s19 { int f0; struct s8 f1; };
struct s19 f19(void) {}

// CHECK: define i64 @f20()
struct s20 { struct s8 f1; int f0; };
struct s20 f20(void) {}

// CHECK: define i64 @f21()
struct s21 { struct {} f1; int f0 : 4; };
struct s21 f21(void) {}

// CHECK: define i64 @f22()
// CHECK: define i64 @f23()
// CHECK: define i64 @f24()
// CHECK: define i128 @f25()
// CHECK: define { float, float } @f26()
// CHECK: define { double, double } @f27()
_Complex char       f22(void) {}
_Complex short      f23(void) {}
_Complex int        f24(void) {}
_Complex long long  f25(void) {}
_Complex float      f26(void) {}
_Complex double     f27(void) {}

// CHECK: define i64 @f28()
struct s28 { _Complex char f0; };
struct s28 f28() {}

// CHECK: define i64 @f29()
struct s29 { _Complex short f0; };
struct s29 f29() {}

// CHECK: define i64 @f30()
struct s30 { _Complex int f0; };
struct s30 f30() {}

struct s31 { char x; };
void f31(struct s31 s) { }
// CHECK: define void @f31(i64 %s.coerce)
// CHECK: %s = alloca %struct.s31, align 8
// CHECK: trunc i64 %s.coerce to i8
// CHECK: store i8 %{{.*}},

struct s32 { double x; };
void f32(struct s32 s) { }
// Expand Homogeneous Aggregate.
// CHECK: @f32(double %{{.*}})

// A composite type larger than 16 bytes should be passed indirectly.
struct s33 { char buf[32*32]; };
void f33(struct s33 s) { }
// CHECK: define void @f33(%struct.s33* %s)

struct s34 { char c; };
void f34(struct s34 s);
void g34(struct s34 *s) { f34(*s); }
// CHECK: @g34(%struct.s34* %s)
// CHECK: %[[a:.*]] = load i8* %{{.*}}
// CHECK: zext i8 %[[a]] to i64
// CHECK: call void @f34(i64 %{{.*}})

/*
 * Check that va_arg accesses stack according to ABI alignment
 */
long long t1(int i, ...) {
    // CHECK: t1
    __builtin_va_list ap;
    __builtin_va_start(ap, i);
    // CHECK-NOT: add i32 %{{.*}} 7
    // CHECK-NOT: and i32 %{{.*}} -8
    long long ll = __builtin_va_arg(ap, long long);
    __builtin_va_end(ap);
    return ll;
}
double t2(int i, ...) {
    // CHECK: t2
    __builtin_va_list ap;
    __builtin_va_start(ap, i);
    // CHECK-NOT: add i32 %{{.*}} 7
    // CHECK-NOT: and i32 %{{.*}} -8
    double ll = __builtin_va_arg(ap, double);
    __builtin_va_end(ap);
    return ll;
}

#include <aarch64_simd.h>

// Homogeneous Vector Aggregate as return type and argument type.
// CHECK: define %struct.int8x16x2_t @f0_0(<16 x i8> %{{.*}}, <16 x i8> %{{.*}})
int8x16x2_t f0_0(int8x16_t a0, int8x16_t a1) {
  return vzipq_s8(a0, a1);
}

// Test direct vector passing.
typedef float T_float32x2 __attribute__ ((__vector_size__ (8)));
typedef float T_float32x4 __attribute__ ((__vector_size__ (16)));
typedef float T_float32x8 __attribute__ ((__vector_size__ (32)));
typedef float T_float32x16 __attribute__ ((__vector_size__ (64)));

// CHECK: define <2 x float> @f1_0(<2 x float> %{{.*}})
T_float32x2 f1_0(T_float32x2 a0) { return a0; }
// CHECK: define <4 x float> @f1_1(<4 x float> %{{.*}})
T_float32x4 f1_1(T_float32x4 a0) { return a0; }
// Vector with length bigger than 16-byte is illegal and is passed indirectly.
// CHECK: define void @f1_2(<8 x float>* noalias sret  %{{.*}}, <8 x float>*)
T_float32x8 f1_2(T_float32x8 a0) { return a0; }
// CHECK: define void @f1_3(<16 x float>* noalias sret %{{.*}}, <16 x float>*)
T_float32x16 f1_3(T_float32x16 a0) { return a0; }

// Testing alignment with aggregates: HFA, aggregates with size <= 16 bytes and
// aggregates with size > 16 bytes.
struct s35
{
   float v[4]; //Testing HFA.
} __attribute__((aligned(16)));
typedef struct s35 s35_with_align;

typedef __attribute__((neon_vector_type(4))) float float32x4_t;
float32x4_t f35(int i, s35_with_align s1, s35_with_align s2) {
// CHECK: define <4 x float> @f35(i32 %i, float %s1.0, float %s1.1, float %s1.2, float %s1.3, float %s2.0, float %s2.1, float %s2.2, float %s2.3)
// CHECK: %s1 = alloca %struct.s35, align 16
// CHECK: %s2 = alloca %struct.s35, align 16
// CHECK: %[[a:.*]] = bitcast %struct.s35* %s1 to <4 x float>*
// CHECK: load <4 x float>* %[[a]], align 16
// CHECK: %[[b:.*]] = bitcast %struct.s35* %s2 to <4 x float>*
// CHECK: load <4 x float>* %[[b]], align 16
  float32x4_t v = vaddq_f32(*(float32x4_t *)&s1,
                            *(float32x4_t *)&s2);
  return v;
}

struct s36
{
   int v[4]; //Testing 16-byte aggregate.
} __attribute__((aligned(16)));
typedef struct s36 s36_with_align;

typedef __attribute__((neon_vector_type(4))) int int32x4_t;
int32x4_t f36(int i, s36_with_align s1, s36_with_align s2) {
// CHECK: define <4 x i32> @f36(i32 %i, i128 %s1.coerce, i128 %s2.coerce)
// CHECK: %s1 = alloca %struct.s36, align 16
// CHECK: %s2 = alloca %struct.s36, align 16
// CHECK: store i128 %s1.coerce, i128* %{{.*}}, align 1
// CHECK: store i128 %s2.coerce, i128* %{{.*}}, align 1
// CHECK: %[[a:.*]] = bitcast %struct.s36* %s1 to <4 x i32>*
// CHECK: load <4 x i32>* %[[a]], align 16
// CHECK: %[[b:.*]] = bitcast %struct.s36* %s2 to <4 x i32>*
// CHECK: load <4 x i32>* %[[b]], align 16
  int32x4_t v = vaddq_s32(*(int32x4_t *)&s1,
                          *(int32x4_t *)&s2);
  return v;
}

struct s37
{
   int v[18]; //Testing large aggregate.
} __attribute__((aligned(16)));
typedef struct s37 s37_with_align;

int32x4_t f37(int i, s37_with_align s1, s37_with_align s2) {
// CHECK: define <4 x i32> @f37(i32 %i, %struct.s37* %s1, %struct.s37* %s2)
// CHECK: %[[a:.*]] = bitcast %struct.s37* %s1 to <4 x i32>*
// CHECK: load <4 x i32>* %[[a]], align 16
// CHECK: %[[b:.*]] = bitcast %struct.s37* %s2 to <4 x i32>*
// CHECK: load <4 x i32>* %[[b]], align 16
  int32x4_t v = vaddq_s32(*(int32x4_t *)&s1,
                          *(int32x4_t *)&s2);
  return v;
}
s37_with_align g37;
int32x4_t caller37() {
// CHECK: caller37
// CHECK: %[[a:.*]] = alloca %struct.s37, align 16
// CHECK: %[[b:.*]] = alloca %struct.s37, align 16
// CHECK: call void @llvm.memcpy
// CHECK: call void @llvm.memcpy
// CHECK: call <4 x i32> @f37(i32 3, %struct.s37* %[[a]], %struct.s37* %[[b]])
  return f37(3, g37, g37);
}
