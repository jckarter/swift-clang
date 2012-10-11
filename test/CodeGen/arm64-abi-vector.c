// RUN: %clang_cc1 -triple arm64-apple-ios -emit-llvm -o - %s | FileCheck %s

#include <stdarg.h>

typedef __attribute__(( ext_vector_type(3) ))  char __char3;
typedef __attribute__(( ext_vector_type(4) ))  char __char4;
typedef __attribute__(( ext_vector_type(5) ))  char __char5;
typedef __attribute__(( ext_vector_type(9) ))  char __char9;
typedef __attribute__(( ext_vector_type(19) )) char __char19;
typedef __attribute__(( ext_vector_type(3) ))  short __short3;
typedef __attribute__(( ext_vector_type(5) ))  short __short5;
typedef __attribute__(( ext_vector_type(3) ))  int __int3;
typedef __attribute__(( ext_vector_type(5) ))  int __int5;
typedef __attribute__(( ext_vector_type(3) ))  double __double3;

double varargs_vec_3c(int fixed, ...) {
// CHECK: varargs_vec_3c
// CHECK: %ap.next = getelementptr i8* %ap.cur, i32 8
// CHECK: bitcast i8* %ap.cur to <3 x i8>*
  va_list ap;
  double sum = fixed;
  va_start(ap, fixed);
  __char3 c3 = va_arg(ap, __char3);
  sum = sum + c3.x + c3.y;
  va_end(ap);
  return sum;
}

double test_3c(__char3 *in) {
// CHECK: test_3c
// CHECK: call double (i32, ...)* @varargs_vec_3c(i32 3, i32 %2)
  return varargs_vec_3c(3, *in);
}

double varargs_vec_4c(int fixed, ...) {
// CHECK: varargs_vec_4c
// CHECK: %ap.next = getelementptr i8* %ap.cur, i32 8
// CHECK: bitcast i8* %ap.cur to <4 x i8>*
  va_list ap;
  double sum = fixed;
  va_start(ap, fixed);
  __char4 c4 = va_arg(ap, __char4);
  sum = sum + c4.x + c4.y;
  va_end(ap);
  return sum;
}

double test_4c(__char4 *in) {
// CHECK: test_4c
// CHECK: call double (i32, ...)* @varargs_vec_4c(i32 4, i32 %3)
  return varargs_vec_4c(4, *in);
}

double varargs_vec_5c(int fixed, ...) {
// CHECK: varargs_vec_5c
// CHECK: %ap.next = getelementptr i8* %ap.cur, i32 8
// CHECK: bitcast i8* %ap.cur to <5 x i8>*
  va_list ap;
  double sum = fixed;
  va_start(ap, fixed);
  __char5 c5 = va_arg(ap, __char5);
  sum = sum + c5.x + c5.y;
  va_end(ap);
  return sum;
}

double test_5c(__char5 *in) {
// CHECK: test_5c
// CHECK: call double (i32, ...)* @varargs_vec_5c(i32 5, <2 x i32> %3)
  return varargs_vec_5c(5, *in);
}

double varargs_vec_9c(int fixed, ...) {
// CHECK: varargs_vec_9c
// CHECK: %ap.next = getelementptr i8* %ap.align, i32 16
// CHECK: bitcast i8* %ap.align to <9 x i8>*
  va_list ap;
  double sum = fixed;
  va_start(ap, fixed);
  __char9 c9 = va_arg(ap, __char9);
  sum = sum + c9.x + c9.y;
  va_end(ap);
  return sum;
}

double test_9c(__char9 *in) {
// CHECK: test_9c
// CHECK: call double (i32, ...)* @varargs_vec_9c(i32 9, <4 x i32> %3)
  return varargs_vec_9c(9, *in);
}

double varargs_vec_19c(int fixed, ...) {
// CHECK: varargs_vec_19c
// CHECK: %ap.next = getelementptr i8* %ap.cur, i32 8
// CHECK: %1 = bitcast i8* %ap.cur to i8**
// CHECK: %2 = load i8** %1
// CHECK: bitcast i8* %2 to <19 x i8>*
  va_list ap;
  double sum = fixed;
  va_start(ap, fixed);
  __char19 c19 = va_arg(ap, __char19);
  sum = sum + c19.x + c19.y;
  va_end(ap);
  return sum;
}

double test_19c(__char19 *in) {
// CHECK: test_19c
// CHECK: call double (i32, ...)* @varargs_vec_19c(i32 19, <19 x i8>* %tmp)
  return varargs_vec_19c(19, *in);
}

double varargs_vec_3s(int fixed, ...) {
// CHECK: varargs_vec_3s
// CHECK: %ap.next = getelementptr i8* %ap.cur, i32 8
// CHECK: %1 = bitcast i8* %ap.cur to <3 x i16>*
  va_list ap;
  double sum = fixed;
  va_start(ap, fixed);
  __short3 c3 = va_arg(ap, __short3);
  sum = sum + c3.x + c3.y;
  va_end(ap);
  return sum;
}

double test_3s(__short3 *in) {
// CHECK: test_3s
// CHECK: call double (i32, ...)* @varargs_vec_3s(i32 3, <2 x i32> %2)
  return varargs_vec_3s(3, *in);
}

double varargs_vec_5s(int fixed, ...) {
// CHECK: varargs_vec_5s
// CHECK: %ap.next = getelementptr i8* %ap.align, i32 16
// CHECK: bitcast i8* %ap.align to <5 x i16>*
  va_list ap;
  double sum = fixed;
  va_start(ap, fixed);
  __short5 c5 = va_arg(ap, __short5);
  sum = sum + c5.x + c5.y;
  va_end(ap);
  return sum;
}

double test_5s(__short5 *in) {
// CHECK: test_5s
// CHECK: call double (i32, ...)* @varargs_vec_5s(i32 5, <4 x i32> %3)
  return varargs_vec_5s(5, *in);
}

double varargs_vec_3i(int fixed, ...) {
// CHECK: varargs_vec_3i
// CHECK: %ap.next = getelementptr i8* %ap.align, i32 16
// CHECK: bitcast i8* %ap.align to <3 x i32>*
  va_list ap;
  double sum = fixed;
  va_start(ap, fixed);
  __int3 c3 = va_arg(ap, __int3);
  sum = sum + c3.x + c3.y;
  va_end(ap);
  return sum;
}

double test_3i(__int3 *in) {
// CHECK: test_3i
// CHECK: call double (i32, ...)* @varargs_vec_3i(i32 3, <4 x i32> %2)
  return varargs_vec_3i(3, *in);
}

double varargs_vec_5i(int fixed, ...) {
// CHECK: varargs_vec_5i
// CHECK: %ap.next = getelementptr i8* %ap.cur, i32 8
// CHECK: %1 = bitcast i8* %ap.cur to i8**
// CHECK: %2 = load i8** %1
// CHECK: bitcast i8* %2 to <5 x i32>*
  va_list ap;
  double sum = fixed;
  va_start(ap, fixed);
  __int5 c5 = va_arg(ap, __int5);
  sum = sum + c5.x + c5.y;
  va_end(ap);
  return sum;
}

double test_5i(__int5 *in) {
// CHECK: test_5i
// CHECK: call double (i32, ...)* @varargs_vec_5i(i32 5, <5 x i32>* %tmp)
  return varargs_vec_5i(5, *in);
}

double varargs_vec_3d(int fixed, ...) {
// CHECK: varargs_vec_3d
// CHECK: %ap.next = getelementptr i8* %ap.cur, i32 8
// CHECK: %1 = bitcast i8* %ap.cur to i8**
// CHECK: %2 = load i8** %1
// CHECK: bitcast i8* %2 to <3 x double>*
  va_list ap;
  double sum = fixed;
  va_start(ap, fixed);
  __double3 c3 = va_arg(ap, __double3);
  sum = sum + c3.x + c3.y;
  va_end(ap);
  return sum;
}

double test_3d(__double3 *in) {
// CHECK: test_3d
// CHECK: call double (i32, ...)* @varargs_vec_3d(i32 3, <3 x double>* %tmp)
  return varargs_vec_3d(3, *in);
}

double varargs_vec(int fixed, ...) {
// CHECK: varargs_vec
  va_list ap;
  double sum = fixed;
  va_start(ap, fixed);
  __char3 c3 = va_arg(ap, __char3);
// CHECK: %ap.next = getelementptr i8* %ap.cur, i32 8
// CHECK: %1 = bitcast i8* %ap.cur to <3 x i8>*
  sum = sum + c3.x + c3.y;
  __char5 c5 = va_arg(ap, __char5);
// CHECK: %ap.next8 = getelementptr i8* %ap.cur7, i32 8
// CHECK: %8 = bitcast i8* %ap.cur7 to <5 x i8>*
  sum = sum + c5.x + c5.y;
  __char9 c9 = va_arg(ap, __char9);
// CHECK: %ap.next16 = getelementptr i8* %ap.align, i32 16
// CHECK: %18 = bitcast i8* %ap.align to <9 x i8>*
  sum = sum + c9.x + c9.y;
  __char19 c19 = va_arg(ap, __char19);
// CHECK: %ap.next24 = getelementptr i8* %ap.cur23, i32 8
// CHECK: %25 = bitcast i8* %ap.cur23 to i8**
// CHECK: %26 = load i8** %25
// CHECK: %27 = bitcast i8* %26 to <19 x i8>*
  sum = sum + c19.x + c19.y;
  __short3 s3 = va_arg(ap, __short3);
// CHECK: %ap.next32 = getelementptr i8* %ap.cur31, i32 8
// CHECK: %34 = bitcast i8* %ap.cur31 to <3 x i16>*
  sum = sum + s3.x + s3.y;
  __short5 s5 = va_arg(ap, __short5);
// CHECK: %ap.next43 = getelementptr i8* %ap.align42, i32 16
// CHECK: %44 = bitcast i8* %ap.align42 to <5 x i16>*
  sum = sum + s5.x + s5.y;
  __int3 i3 = va_arg(ap, __int3);
// CHECK: %ap.next52 = getelementptr i8* %ap.align51, i32 16
// CHECK: %54 = bitcast i8* %ap.align51 to <3 x i32>*
  sum = sum + i3.x + i3.y;
  __int5 i5 = va_arg(ap, __int5);
// CHECK: %ap.next60 = getelementptr i8* %ap.cur59, i32 8
// CHECK: %61 = bitcast i8* %ap.cur59 to i8**
// CHECK: %62 = load i8** %61
// CHECK: %63 = bitcast i8* %62 to <5 x i32>*
  sum = sum + i5.x + i5.y;
  __double3 d3 = va_arg(ap, __double3);
// CHECK: %ap.next66 = getelementptr i8* %ap.cur65, i32 8
// CHECK: %70 = bitcast i8* %ap.cur65 to i8**
// CHECK: %71 = load i8** %70
// CHECK: %72 = bitcast i8* %71 to <3 x double>*
  sum = sum + d3.x + d3.y;
  va_end(ap);
  return sum;
}

double test(__char3 *c3, __char5 *c5, __char9 *c9, __char19 *c19,
            __short3 *s3, __short5 *s5, __int3 *i3, __int5 *i5,
            __double3 *d3) {
  double ret = varargs_vec(3, *c3, *c5, *c9, *c19, *s3, *s5, *i3, *i5, *d3);
// CHECK: call double (i32, ...)* @varargs_vec(i32 3, i32 %15, <2 x i32> %17, <4 x i32> %19, <19 x i8>* %tmp, <2 x i32> %21, <4 x i32> %23, <4 x i32> %25, <5 x i32>* %tmp20, <3 x double>* %tmp21)
  return ret;
}

__attribute__((noinline)) double args_vec_3c(int fixed, __char3 c3) {
// CHECK: args_vec_3c
// CHECK: %c3 = alloca <3 x i8>, align 4
// CHECK: %0 = bitcast <3 x i8>* %c3 to i32*
// CHECK: store i32 %c3.coerce, i32* %0
  double sum = fixed;
  sum = sum + c3.x + c3.y;
  return sum;
}

double fixed_3c(__char3 *in) {
// CHECK: fixed_3c
// CHECK: call double @args_vec_3c(i32 3, i32 %2)
  return args_vec_3c(3, *in);
}

__attribute__((noinline)) double args_vec_5c(int fixed, __char5 c5) {
// CHECK: args_vec_5c
// CHECK: %c5 = alloca <5 x i8>, align 8
// CHECK: %0 = bitcast <5 x i8>* %c5 to <2 x i32>*
// CHECK: store <2 x i32> %c5.coerce, <2 x i32>* %0, align 1
  double sum = fixed;
  sum = sum + c5.x + c5.y;
  return sum;
}

double fixed_5c(__char5 *in) {
// CHECK: fixed_5c
// CHECK: call double @args_vec_5c(i32 5, <2 x i32> %3)
  return args_vec_5c(5, *in);
}

__attribute__((noinline)) double args_vec_9c(int fixed, __char9 c9) {
// CHECK: args_vec_9c
// CHECK: %c9 = alloca <9 x i8>, align 16
// CHECK: %0 = bitcast <9 x i8>* %c9 to <4 x i32>*
// CHECK: store <4 x i32> %c9.coerce, <4 x i32>* %0, align 1
  double sum = fixed;
  sum = sum + c9.x + c9.y;
  return sum;
}

double fixed_9c(__char9 *in) {
// CHECK: fixed_9c
// CHECK: call double @args_vec_9c(i32 9, <4 x i32> %3)
  return args_vec_9c(9, *in);
}

__attribute__((noinline)) double args_vec_19c(int fixed, __char19 c19) {
// CHECK: args_vec_19c
// CHECK: %c19 = load <19 x i8>* %0, align 32
  double sum = fixed;
  sum = sum + c19.x + c19.y;
  return sum;
}

double fixed_19c(__char19 *in) {
// CHECK: fixed_19c
// CHECK: call double @args_vec_19c(i32 19, <19 x i8>* %tmp)
  return args_vec_19c(19, *in);
}

__attribute__((noinline)) double args_vec_3s(int fixed, __short3 c3) {
// CHECK: args_vec_3s
// CHECK: %c3 = alloca <3 x i16>, align 8
// CHECK: %0 = bitcast <3 x i16>* %c3 to <2 x i32>*
// CHECK: store <2 x i32> %c3.coerce, <2 x i32>* %0, align 1
  double sum = fixed;
  sum = sum + c3.x + c3.y;
  return sum;
}

double fixed_3s(__short3 *in) {
// CHECK: fixed_3s
// CHECK: call double @args_vec_3s(i32 3, <2 x i32> %2)
  return args_vec_3s(3, *in);
}

__attribute__((noinline)) double args_vec_5s(int fixed, __short5 c5) {
// CHECK: args_vec_5s
// CHECK: %c5 = alloca <5 x i16>, align 16
// CHECK: %0 = bitcast <5 x i16>* %c5 to <4 x i32>*
// CHECK: store <4 x i32> %c5.coerce, <4 x i32>* %0, align 1
  double sum = fixed;
  sum = sum + c5.x + c5.y;
  return sum;
}

double fixed_5s(__short5 *in) {
// CHECK: fixed_5s
// CHECK: call double @args_vec_5s(i32 5, <4 x i32> %3)
  return args_vec_5s(5, *in);
}

__attribute__((noinline)) double args_vec_3i(int fixed, __int3 c3) {
// CHECK: args_vec_3i
// CHECK: %c3 = alloca <3 x i32>, align 16
// CHECK: %0 = bitcast <3 x i32>* %c3 to <4 x i32>*
// CHECK: store <4 x i32> %c3.coerce, <4 x i32>* %0, align 1
  double sum = fixed;
  sum = sum + c3.x + c3.y;
  return sum;
}

double fixed_3i(__int3 *in) {
// CHECK: fixed_3i
// CHECK: call double @args_vec_3i(i32 3, <4 x i32> %2)
  return args_vec_3i(3, *in);
}

__attribute__((noinline)) double args_vec_5i(int fixed, __int5 c5) {
// CHECK: args_vec_5i
// CHECK: %c5 = load <5 x i32>* %0, align 32
  double sum = fixed;
  sum = sum + c5.x + c5.y;
  return sum;
}

double fixed_5i(__int5 *in) {
// CHECK: fixed_5i
// CHECK: call double @args_vec_5i(i32 5, <5 x i32>* %tmp)
  return args_vec_5i(5, *in);
}

__attribute__((noinline)) double args_vec_3d(int fixed, __double3 c3) {
// CHECK: args_vec_3d
// CHECK: %castToVec4 = bitcast <3 x double>* %0 to <4 x double>*
// CHECK: %loadVec4 = load <4 x double>* %castToVec4
// CHECK: %c3 = shufflevector <4 x double> %loadVec4, <4 x double> undef, <3 x i32> <i32 0, i32 1, i32 2>
  double sum = fixed;
  sum = sum + c3.x + c3.y;
  return sum;
}

double fixed_3d(__double3 *in) {
// CHECK: fixed_3d
// CHECK: call double @args_vec_3d(i32 3, <3 x double>* %tmp)
  return args_vec_3d(3, *in);
}
