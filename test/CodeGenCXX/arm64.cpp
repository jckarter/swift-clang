// RUN: %clang_cc1 %s -triple=arm64-apple-ios -emit-llvm -o - | FileCheck %s

// __cxa_guard_acquire argument is 64-bit
// rdar://11540122
struct A {
  A();
};

void f() {
  // CHECK: call i32 @__cxa_guard_acquire(i64*
  static A a;
}
