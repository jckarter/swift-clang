// RUN: %clang_cc1 -fsyntax-only -verify %s

void f1(void) {
  int x = 0;
/* expected-error {{'#pragma fp_contract' should only appear at file scope or at the start of a compound expression}} */ #pragma STDC FP_CONTRACT ON
}
