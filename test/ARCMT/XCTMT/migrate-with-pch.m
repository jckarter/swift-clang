// RUN: %clang -target x86_64-apple-darwin10 -x objective-c-header -F %S -fobjc-exceptions %S/migrate-with-pch.h -o %t.prefix.h.pch
// RUN: %clang -target x86_64-apple-darwin10 --migrate-xct -F %S -fobjc-exceptions %s -include %t.prefix.h -o %t.remap
// RUN: c-arcmt-test %t.remap | arcmt-test -verify-transformed-files %S/migrate-with-pch.h.result %s.result

@implementation test_kitTests

- (void)testExample
{
  SenSelfTestMain();
  id x = SenTestCaseDidStartNotification;
  STFail(@"fail");
}

@end
