// RUN: %clang_cc1 -fsyntax-only -fblocks -I %S/Inputs %s -verify

#include "nullability-pragmas-1.h"
#include "nullability-pragmas-2.h"
#include "nullability-pragmas-generics-1.h"

#if !__has_feature(assume_nonnull)
#  error assume_nonnull feature is not set
#endif

void test_pragmas_1(__nonnull A *a) {
  f1(0); // okay: no nullability annotations
  f2(0); // expected-warning{{null passed to a callee that requires a non-null argument}}
  f3(0); // expected-warning{{null passed to a callee that requires a non-null argument}}
  f4(0); // expected-warning{{null passed to a callee that requires a non-null argument}}
  f5(0); // expected-warning{{null passed to a callee that requires a non-null argument}}
  f6(0); // expected-warning{{null passed to a callee that requires a non-null argument}}
  f7(0); // okay
  f8(0); // okay
  f9(0); // expected-warning{{null passed to a callee that requires a non-null argument}}
  f10(0); // expected-warning{{null passed to a callee that requires a non-null argument}}
  f11(0); // okay
  f12(0); // okay
  [a method1:0]; // expected-warning{{null passed to a callee that requires a non-null argument}}
  f17(a); // expected-error{{no matching function for call to 'f17'}}
  [a method3: a]; // expected-error{{cannot initialize a parameter of type '__nullable NSError ** __nullable' with an lvalue of type '__nonnull A *'}}
  [a method4: a]; // expected-error{{cannot initialize a parameter of type '__nullable NSErrorPtr * __nullable' (aka 'NSError **') with an lvalue of type '__nonnull A *'}}
  
  float *ptr;
  ptr = f13(); // expected-error{{assigning to 'float *' from incompatible type '__nonnull int_ptr' (aka 'int *')}}
  ptr = f14(); // expected-error{{assigning to 'float *' from incompatible type '__nonnull A *'}}
  ptr = [A aWithA:a]; // expected-error{{assigning to 'float *' from incompatible type '__nonnull A *'}}
  ptr = [a method1:a]; // expected-error{{assigning to 'float *' from incompatible type '__nonnull A *'}}
  ptr = a.aProp; // expected-error{{assigning to 'float *' from incompatible type '__nonnull A *'}}
  ptr = global_int_ptr; // expected-error{{assigning to 'float *' from incompatible type '__nonnull int *'}}
  ptr = f15(); // expected-error{{assigning to 'float *' from incompatible type '__null_unspecified int *'}}
  ptr = f16(); // expected-error{{assigning to 'float *' from incompatible type '__null_unspecified A *'}}
  ptr = [a method2]; // expected-error{{assigning to 'float *' from incompatible type '__null_unspecified A *'}}
}

void test_pragmas_generics(void) {
  float *fp;

  NSGeneric<C *> *genC;
  fp = [genC tee]; // expected-error{{from incompatible type 'C *'}}
  fp = [genC maybeTee]; // expected-error{{from incompatible type '__nullable C *'}}

  Generic_with_C genC2;
  fp = genC2; // expected-error{{from incompatible type 'Generic_with_C' (aka 'NSGeneric<C *> *')}}
}