// RUN: %clang_cc1 -fapinotes -fapinotes-cache-path=%t/APINotesCache -fsyntax-only -I %S/Inputs/Headers -F %S/Inputs/Frameworks %s -verify

#include "HeaderLib.h"

int main() {
  custom_realloc(0, 0); // expected-warning{{null passed to a callee that requires a non-null argument}}
  int i = 0;
  do_something_with_pointers(&i, 0);
  do_something_with_pointers(0, &i); // expected-warning{{null passed to a callee that requires a non-null argument}}

  float *fp = global_int; // expected-warning{{incompatible pointer types initializing 'float *' with an expression of type 'int * _Nonnull'}}
  return 0;
}

