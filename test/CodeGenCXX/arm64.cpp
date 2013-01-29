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

// ARM64 uses the C++11 definition of POD.
// rdar://12650514
namespace test1 {
  // This class is POD in C++11 and cannot have objects allocated in
  // its tail-padding.
  struct ABase {};
  struct A : ABase {
    int x;
    char c;
  };

  struct B : A {
    char d;
  };

  int test() {
    return sizeof(B);
  }
  // CHECK: define i32 @_ZN5test14testEv()
  // CHECK: ret i32 12
}
