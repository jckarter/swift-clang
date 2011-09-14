// RUN: %clang_cc1  -fsyntax-only -verify %s
// rdar://10111397

typedef int NSNumber;

int main() {
  NSNumber * N = @3.1415926535;  // expected-error {{NSNumber must be available for use of objective-c literals}}
  NSNumber *noNumber = @__yes; // expected-error {{NSNumber must be available for use of objective-c literals}}
}
