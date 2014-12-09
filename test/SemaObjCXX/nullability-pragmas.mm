// RUN: %clang_cc1 -fsyntax-only -fblocks -I %S/Inputs %s -verify

#include "nullability-pragmas-1.h"
#include "nullability-pragmas-2.h"

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

  float *ptr;
  ptr = f13(); // expected-error{{assigning to 'float *' from incompatible type '__nonnull int_ptr' (aka 'int *')}}
  ptr = f14(); // expected-error{{assigning to 'float *' from incompatible type '__nonnull A *'}}
  ptr = [a method1:a]; // expected-error{{assigning to 'float *' from incompatible type '__nonnull A *'}}
  ptr = a.aProp; // expected-error{{assigning to 'float *' from incompatible type '__nonnull A *'}}
  ptr = global_int_ptr; // expected-error{{assigning to 'float *' from incompatible type '__nonnull int *'}}
}
