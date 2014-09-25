// RUN: %clang_cc1 -fapinotes -fsyntax-only -I %S/Inputs/Headers -F %S/Inputs/Frameworks %s -verify

#include "HeaderLib.h"

int main() {
  custom_realloc(0, 0); // expected-warning{{null passed to a callee that requires a non-null argument}}
  return 0;
}

