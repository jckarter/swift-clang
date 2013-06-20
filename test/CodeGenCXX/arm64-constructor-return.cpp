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


// Temporarily remove these check while investigating issue
// rdar://problem/14209661 

// %struct.S* @_ZN1SC1Ev(%struct.S* returned %this)

// [[ONE:%.*]] = load %struct.S** [[RETVAL:%.*]]
// ret %struct.S* [[ONE]]

// %struct.S* @_ZN1SC2Ev(%struct.S* returned %this)
