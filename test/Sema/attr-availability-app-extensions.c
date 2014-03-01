// RUN: %clang_cc1 -triple x86_64-apple-macosx10.9.0 -fsyntax-only -fapplication-extensions %s -verify
// RUN: %clang_cc1 -triple armv7-apple-ios9.0 -fsyntax-only -fapplication-extensions %s -verify

 __attribute__((availability(macosx_app_extension,unavailable)))
 __attribute__((availability(ios_app_extension,unavailable)))
void f0(int); // expected-note {{'f0' has been explicitly marked unavailable here}}

void test() {
  f0(1); // expected-error {{'f0' is unavailable: not available on}}
}

