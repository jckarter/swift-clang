// RUN: %clang_cc1 %s -triple=arm64-apple-ios7.0.0 -Wno-return-type -emit-llvm -o - | FileCheck %s
// rdar://12162905

struct S {
  S();
  int iField;
};

S::S() {
  if (iField)
    return -1;
};

// CHECK: [[IFEND:%.*]]
// CHECK: [[ONE:%.*]] = load %struct.S** [[RETVAL:%.*]]
// CHECK: ret %struct.S* [[ONE]]
