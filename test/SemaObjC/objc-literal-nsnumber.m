// RUN: %clang_cc1  -fsyntax-only -verify %s
// rdar://10111397

#if __LP64__ || (TARGET_OS_EMBEDDED && !TARGET_OS_IPHONE) || TARGET_OS_WIN32 || NS_BUILD_32_LIKE_64
typedef unsigned long NSUInteger;
#else
typedef unsigned int NSUInteger;
#endif

typedef int NSNumber;

int main() {
  NSNumber * N = @3.1415926535;  // expected-error {{NSNumber must be available for use of objective-c literals}}
  NSNumber *noNumber = @__yes; // expected-error {{NSNumber must be available for use of objective-c literals}}
}
