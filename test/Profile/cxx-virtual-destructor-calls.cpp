// RUN: %clang_cc1 -triple %itanium_abi_triple -emit-llvm -main-file-name cxx-virtual-destructor-calls.cpp %s -o - -fprofile-instr-generate | FileCheck %s

struct Member {
  ~Member();
};

struct A {
  virtual ~A();
};

struct B : A {
  Member m;
  virtual ~B();
};

// Base dtor
// CHECK: @__prf_nm__ZN1BD2Ev = private constant [9 x i8] c"_ZN1BD2Ev"

// Complete dtor must not be instrumented
// CHECK-NOT: @__prf_nm__ZN1BD1Ev = private constant [9 x i8] c"_ZN1BD1Ev"

// Deleting dtor must not be instrumented
// CHECK-NOT: @__prf_nm__ZN1BD0Ev = private constant [9 x i8] c"_ZN1BD0Ev"

// Base dtor counters and profile data
// CHECK: @__prf_cn__ZN1BD2Ev = private global [1 x i64] zeroinitializer
// CHECK: @__prf_dt__ZN1BD2Ev =

// Complete dtor counters and profile data must absent
// CHECK-NOT: @__prf_cn__ZN1BD1Ev = private global [1 x i64] zeroinitializer
// CHECK-NOT: @__prf_dt__ZN1BD1Ev =

// Deleting dtor counters and profile data must absent
// CHECK-NOT: @__prf_cn__ZN1BD0Ev = private global [1 x i64] zeroinitializer
// CHECK-NOT: @__prf_dt__ZN1BD0Ev =

B::~B() { }
