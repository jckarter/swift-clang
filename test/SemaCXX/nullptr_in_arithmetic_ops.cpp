// RUN: %clang_cc1 -fsyntax-only -fblocks -std=c++0x -verify %s

void f() {
  int a;
  bool b;

  a = 0 ? nullptr + a : a + nullptr; // expected-error 2{{invalid operands to binary expression}}
  a = 0 ? nullptr - a : a - nullptr; // expected-error 2{{invalid operands to binary expression}}
  a = 0 ? nullptr / a : a / nullptr; // expected-error 2{{invalid operands to binary expression}}
  a = 0 ? nullptr * a : a * nullptr; // expected-error 2{{invalid operands to binary expression}}
  a = 0 ? nullptr >> a : a >> nullptr; // expected-error 2{{invalid operands to binary expression}}
  a = 0 ? nullptr << a : a << nullptr; // expected-error 2{{invalid operands to binary expression}}
  a = 0 ? nullptr % a : a % nullptr; // expected-error 2{{invalid operands to binary expression}}
  a = 0 ? nullptr & a : a & nullptr; // expected-error 2{{invalid operands to binary expression}}
  a = 0 ? nullptr | a : a | nullptr; // expected-error 2{{invalid operands to binary expression}}
  a = 0 ? nullptr ^ a : a ^ nullptr; // expected-error 2{{invalid operands to binary expression}}

  // Using two nullptrs should only give one error instead of two.
  a = nullptr + nullptr; // expected-error{{invalid operands to binary expression}}
  a = nullptr - nullptr; // expected-error{{invalid operands to binary expression}}
  a = nullptr / nullptr; // expected-error{{invalid operands to binary expression}}
  a = nullptr * nullptr; // expected-error{{invalid operands to binary expression}}
  a = nullptr >> nullptr; // expected-error{{invalid operands to binary expression}}
  a = nullptr << nullptr; // expected-error{{invalid operands to binary expression}}
  a = nullptr % nullptr; // expected-error{{invalid operands to binary expression}}
  a = nullptr & nullptr; // expected-error{{invalid operands to binary expression}}
  a = nullptr | nullptr; // expected-error{{invalid operands to binary expression}}
  a = nullptr ^ nullptr; // expected-error{{invalid operands to binary expression}}

  a += nullptr; // expected-error{{invalid operands to binary expression}}
  a -= nullptr; // expected-error{{invalid operands to binary expression}}
  a /= nullptr; // expected-error{{invalid operands to binary expression}}
  a *= nullptr; // expected-error{{invalid operands to binary expression}}
  a >>= nullptr; // expected-error{{invalid operands to binary expression}}
  a <<= nullptr; // expected-error{{invalid operands to binary expression}}
  a %= nullptr; // expected-error{{invalid operands to binary expression}}
  a &= nullptr; // expected-error{{invalid operands to binary expression}}
  a |= nullptr; // expected-error{{invalid operands to binary expression}}
  a ^= nullptr; // expected-error{{invalid operands to binary expression}}

  b = a < nullptr || nullptr < a; // expected-error 2{{invalid operands to binary expression}}
  b = a > nullptr || nullptr > a; // expected-error 2{{invalid operands to binary expression}}
  b = a <= nullptr || nullptr <= a; // expected-error 2{{invalid operands to binary expression}}
  b = a >= nullptr || nullptr >= a; // expected-error 2{{invalid operands to binary expression}}
  b = a == nullptr || nullptr == a; // expected-error 2{{invalid operands to binary expression}}
  b = a != nullptr || nullptr != a; // expected-error 2{{invalid operands to binary expression}}

  b = &a < nullptr || nullptr < &a || &a > nullptr || nullptr > &a;
  b = &a <= nullptr || nullptr <= &a || &a >= nullptr || nullptr >= &a;
  b = &a == nullptr || nullptr == &a || &a != nullptr || nullptr != &a;

  b = 0 == a;
  b = 0 == &a;

  b = ((nullptr)) != a;  // expected-error{{invalid operands to binary expression}}

  void (^c)();
  c = nullptr;
  b = c == nullptr || nullptr == c || c != nullptr || nullptr != c;
  
  class X;
  void (X::*d) ();
  d = nullptr;
  b = d == nullptr || nullptr == d || d != nullptr || nullptr != d;
}
