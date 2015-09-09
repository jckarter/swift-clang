// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -fapple-kext -fapple-kext-vtable-mitigation -emit-llvm %s -o - | FileCheck %s

// rdar://21452793

namespace test0 {
  struct A {
    virtual void foo();
  };

  // CHECK-LABEL: @_ZN5test01aEPNS_1AE
  void a(A *a) {
    // CHECK: [[T0:%.*]] = load [[A:%.*]]*, [[A]]** {{.*}}, align 8
    // CHECK: [[T1:%.*]] = ptrtoint [[A]]* [[T0]] to i64
    // CHECK: [[T2:%.*]] = or i64 [[T1]], -9223372036854775808
    // CHECK: [[T3:%.*]] = inttoptr i64 [[T2]] to void ([[A]]*)***
    // CHECK: [[T4:%.*]] = load void ([[A]]*)**, void ([[A]]*)*** [[T3]], align 8
    // CHECK: [[T5:%.*]] = getelementptr inbounds void ([[A]]*)*, void ([[A]]*)** [[T4]], i64 0
    // CHECK: [[T6:%.*]] = load void ([[A]]*)*, void ([[A]]*)** [[T5]], align 8
    // CHECK: call void [[T6]]([[A]]* [[T0]])
    a->foo();
  }
}

namespace test1 {
  struct A {
    int x;
  };
  struct B : virtual A {
  };

  // CHECK-LABEL: @_ZN5test11aEPNS_1BE
  void a(B *b) {
    // CHECK: [[T0:%.*]] = load [[B:%.*]]*, [[B]]** {{.*}}, align 8
    // CHECK: [[T1:%.*]] = ptrtoint [[B]]* [[T0]] to i64
    // CHECK: [[T2:%.*]] = or i64 [[T1]], -9223372036854775808
    // CHECK: [[T3:%.*]] = inttoptr i64 [[T2]] to i8**
    // CHECK: [[T4:%.*]] = load i8*, i8** [[T3]], align 8
    // CHECK: [[T5:%.*]] = getelementptr i8, i8* [[T4]], i64 -24
    // CHECK: [[T6:%.*]] = bitcast i8* [[T5]] to i64*
    // CHECK: [[T7:%.*]] = load i64, i64* [[T6]], align 8
    // CHECK: [[T8:%.*]] = bitcast [[B]]* [[T0]] to i8*
    // CHECK: [[T9:%.*]] = getelementptr inbounds i8, i8* [[T8]], i64 [[T7]]
    // CHECK: [[T10:%.*]] = bitcast i8* [[T9]] to [[A:.*]]*
    // CHECK: [[T11:%.*]] = getelementptr inbounds [[A]], [[A]]* [[T10]], i32 0, i32 0
    // CHECK: store i32 7, i32* [[T11]], align 4
    b->x = 7;
  }
}
