// RUN: %clang_cc1 -O1 -triple x86_64-none-linux-gnu -emit-llvm -gline-tables-only %s -o - | FileCheck %s

// There is no exception to handle here, lifetime.end is not a destructor,
// so there is no need have cleanup dest slot related code
// CHECK-LABEL: define i32 @test
int test() {
  int x = 3;
  int *volatile p = &x;
  return *p;
// CHECK: [[X:%.*]] = alloca i32
// CHECK: [[P:%.*]] = alloca i32*
// CHECK: call void @llvm.lifetime.start(i64 4, i8* %{{.*}})
// CHECK: call void @llvm.lifetime.start(i64 8, i8* %{{.*}})
// CHECK-NOT: store i32 %{{.*}}, i32* %cleanup.dest.slot
}
