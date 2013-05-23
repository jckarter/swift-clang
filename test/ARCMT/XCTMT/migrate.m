// RUN: %clang_cc1 -migrate -migration-for-xct -F %S -fobjc-exceptions %s -o %t.remap
// RUN: c-arcmt-test %t.remap | arcmt-test -verify-transformed-files %s.result

#import <SenTestingKit/SenTestingKit.h>

@interface test_kitTests : SenTestCase

@end

@implementation test_kitTests

- (void)setUp
{
  STAssertEquals(0, 0, 0  );
  STAssertEquals(0, 0, 0, 1);
  STAssertEquals(0, 0, nil);
}

typedef SenTestCase TC;
void foo(SenTestCase *tc);

-(SenTestCase *)meth:(SenTestCase *)tc { return 0; }

- (void)testExample
{
  SenTestCase *p;
  [SenTestCase class];
  SenSelfTestMain();
  id x = SenTestCaseDidStartNotification;
  STFail(@"fail");
  STFail(nil);
}

@end
