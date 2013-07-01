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

// CHECK: %struct.S* @_ZN1SC1Ev(%struct.S* returned %this)
// CHECK: store %struct.S* %this, %struct.S** %this.addr
// CHECK: [[THIS1:%.*]] = load %struct.S** %this.addr
// CHECK: ret %struct.S* [[THIS1]]

// CHECK: %struct.S* @_ZN1SC2Ev(%struct.S* returned %this)
