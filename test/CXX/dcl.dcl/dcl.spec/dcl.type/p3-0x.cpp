// RUN: %clang_cc1 -std=c++0x -fsyntax-only -verify %s

// FIXME: when clang supports alias-declarations.
#if 0
using X = struct { // ok
};
#endif

class K {
  virtual ~K();
  // FIXME: the diagnostic here isn't very good
  operator struct S {} (); // expected-error 2{{}}
};

void f() {
  int arr[3] = {1,2,3};

  for (struct S { S(int) {} } s : arr) { // expected-error {{types may not be defined in a for range declaration}}
  }

  new struct T {}; // expected-error {{allocation of incomplete type}} expected-note {{forward declaration}}

  // FIXME: the diagnostic here isn't very good
  try {} catch (struct U {}); // expected-error 3{{}} expected-note 2{{}}

  (void)(struct V { V(int); })0; // expected-error {{'V' can not be defined in a type specifier}}

  (void)dynamic_cast<struct W {}*>((K*)0); // expected-error {{'W' can not be defined in a type specifier}}
  (void)static_cast<struct X {}*>(0); // expected-error {{'X' can not be defined in a type specifier}}
  (void)reinterpret_cast<struct Y {}*>(0); // expected-error {{'Y' can not be defined in a type specifier}}
  (void)const_cast<struct Z {}*>((const Z*)0); // expected-error {{'Z' can not be defined in a type specifier}}
}

void g() throw (struct Ex {}) { // expected-error {{'Ex' can not be defined in a type specifier}}
}

// FIXME: this currently gives a strange error because alignas is not recognised as a keyword yet.
int alignas(struct Aa {}) x; // expected-error {{'Aa' can not be defined in a parameter type}} expected-error {{expected function body}}

int a = sizeof(struct So {}); // expected-error {{'So' can not be defined in a type specifier}}
int b = alignof(struct Ao {}); // expected-error {{'Ao' can not be defined in a type specifier}}

namespace std { struct type_info; }
const std::type_info &ti = typeid(struct Ti {}); // expected-error {{'Ti' can not be defined in a type specifier}}
