// RUN: %clang_cc1 -triple x86_64-apple-macosx10.9.0 -fsyntax-only -fapplication-extensions %s -verify
// RUN: %clang_cc1 -triple armv7-apple-ios9.0 -fsyntax-only -fapplication-extensions %s -verify

#if __has_feature(attribute_availability_app_extensions)
 __attribute__((availability(macosx_app_extensions,unavailable)))
 __attribute__((availability(ios_app_extensions,unavailable)))
#endif
void f0(int); // expected-note {{'f0' has been explicitly marked unavailable here}}

void test() {
  f0(1); // expected-error {{'f0' is unavailable: not available on}}
}

