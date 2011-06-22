// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -emit-llvm -fobjc-nonfragile-abi -fexceptions -fobjc-exceptions -o - %s | FileCheck %s

// rdar://problem/8535238
// CHECK: declare void @objc_exception_rethrow()

void protos() {
  extern void foo();
  @try {
    foo();
  } @catch (id e) {
    @throw;
  }
}

void throwing() {
  @throw(@"error!");
}

// rdar://problem/9431547
void die(void) __attribute__((nothrow, noreturn));
void test2(void) {
  @try {
    die();
  } @finally {
    extern void test2_helper(void);
    test2_helper();
  }

  // CHECK: define void @test2()
  // CHECK-NOT: call void @test2_helper()
}
