// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -fobjc-runtime=macosx-fragile-10.5 -fobjc-gc -emit-llvm -o %t %s
// RUN: not grep objc_assign_global %t
// RUN: not grep objc_assign_strongCast %t
// RUN: %clang_cc1 -x objective-c++ -triple x86_64-apple-darwin10 -fobjc-runtime=macosx-fragile-10.5 -fobjc-gc -emit-llvm -o %t %s
// RUN: not grep objc_assign_global %t
// RUN: not grep objc_assign_strongCast %t

@interface A
@end

typedef struct s0 {
  A *a[4];
} T;

T g0;

void f0(id x) {
  g0.a[0] = x;
}

void f1(id x) {
  ((T*) &g0)->a[0] = x;
}

void f2(unsigned idx)
{
   id *keys;
   keys[idx] = 0;
}

