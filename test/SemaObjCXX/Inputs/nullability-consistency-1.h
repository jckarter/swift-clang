void f1(int *ptr); // expected-warning{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}

void f2(__nonnull int *);

#include "nullability-consistency-2.h"

void f3(int *ptr) { // expected-warning{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}
  int *other = ptr; // shouldn't warn
}

class X {
  void mf(int *ptr); // expected-warning{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}
  int X:: *memptr; // expected-warning{{member pointer is missing a nullability type specifier (__nonnull or __nullable)}}
};
