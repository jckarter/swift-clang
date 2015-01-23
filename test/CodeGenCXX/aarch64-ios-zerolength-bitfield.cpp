// REQUIRES: aarch64-registered-target
// RUN: %clang_cc1 -triple aarch64-apple-ios7.0 -x c++ -std=c++1z %s -verify
// expected-no-diagnostics

// FIXME: this should be merged into the aapcs test if the SWBs go well.

#include <stddef.h>

struct t1
{
  int foo : 1;
  char : 0;
  char bar;

};
static_assert(offsetof(struct t1, bar) == 1);
static_assert(sizeof(struct t1) == 4);

struct t2
{
  int foo : 1;
  short : 0;
  char bar;
};
static_assert(offsetof(struct t2, bar) == 2);
static_assert(sizeof(struct t2) == 4);

struct t3
{
  int foo : 1;
  int : 0;
  char bar;
};
static_assert(offsetof(struct t3, bar) == 4);
static_assert(sizeof(struct t3) == 8);

struct t4
{
  int foo : 1;
  long : 0;
  char bar;
};
static_assert(offsetof(struct t4, bar) == 8);
static_assert(sizeof(struct t4) == 12);

struct t5
{
  int foo : 1;
  long long : 0;
  char bar;
};
static_assert(offsetof(struct t5, bar) == 8);
static_assert(sizeof(struct t5) == 12);

struct t6
{
  int foo : 1;
  char : 0;
  char bar : 1;
  char bar2;
};
static_assert(offsetof(struct t6, bar2) == 2);
static_assert(sizeof(struct t6) == 4);

struct t7
{
  int foo : 1;
  short : 0;
  char bar1 : 1;
  char bar2;
};
static_assert(offsetof(struct t7, bar2) == 3);
static_assert(sizeof(struct t7) == 4);

struct t8
{
  int foo : 1;
  int : 0;
  char bar1 : 1;
  char bar2;
};
static_assert(offsetof(struct t8, bar2) == 5);
static_assert(sizeof(struct t8) == 8);

struct t9
{
  int foo : 1;
  long : 0;
  char bar1 : 1;
  char bar2;
};
static_assert(offsetof(struct t9, bar2) == 9);
static_assert(sizeof(struct t9) == 12);

struct t10
{
  int foo : 1;
  long long : 0;
  char bar1 : 1;
  char bar2;
};
static_assert(offsetof(struct t10, bar2) == 9);
static_assert(sizeof(struct t10) == 12);

struct t11
{
  int foo : 1;
  long long : 0;
  char : 0;
  char bar1 : 1;
  char bar2;
};
static_assert(offsetof(struct t11, bar2) == 9);
static_assert(sizeof(struct t11) == 12);

struct t12
{
  int foo : 1;
  char : 0;
  long long : 0;
  char : 0;
  char bar;
};
static_assert(offsetof(struct t12, bar) == 8);
static_assert(sizeof(struct t12) == 12);

struct t13
{
  char foo;
  long : 0;
  char bar;
};
static_assert(offsetof(struct t13, bar) == 8);
static_assert(sizeof(struct t13) == 9);

struct t14
{
  char foo1;
  int : 0;
  char foo2 : 1;
  short foo3 : 16;
  char : 0;
  short foo4 : 16;
  char bar1;
  int : 0;
  char bar2;
};
static_assert(offsetof(struct t14, bar1) == 10);
static_assert(offsetof(struct t14, bar2) == 12);
static_assert(sizeof(struct t14) == 14);

struct t15
{
  char foo;
  char : 0;
  int : 0;
  char bar;
  long : 0;
  char : 0;
};
static_assert(offsetof(struct t15, bar) == 4);
static_assert(sizeof(struct t15) == 8);

struct t16
{
  long : 0;
  char bar;
};
static_assert(offsetof(struct t16, bar) == 0);
static_assert(sizeof(struct t16) == 1);

struct t17
{
  char foo;
  long : 0;
  long : 0;
  char : 0;
  char bar;
};
static_assert(offsetof(struct t17, bar) == 8);
static_assert(sizeof(struct t17) == 9);

struct t18
{
  long : 0;
  long : 0;
  char : 0;
};
static_assert(sizeof(struct t18) == 1);

struct t19
{
  char foo1;
  long foo2 : 1;
  char : 0;
  long foo3 : 32;
  char bar;
};
static_assert(offsetof(struct t19, bar) == 6);
static_assert(sizeof(struct t19) == 8);

struct t20
{
  short : 0;
  int foo : 1;
  long : 0;
  char bar;
};
static_assert(offsetof(struct t20, bar) == 8);
static_assert(sizeof(struct t20) == 12);

struct t21
{
  short : 0;
  int foo1 : 1;
  char : 0;
  int foo2 : 16;
  long : 0;
  char bar1;
  int bar2;
  long bar3;
  char foo3 : 8;
  char : 0;
  long : 0;
  int foo4 : 32;
  short foo5: 1;
  long bar4;
  short foo6: 16;
  short foo7: 16;
  short foo8: 16;
};
static_assert(offsetof(struct t21, bar1) == 8);
static_assert(offsetof(struct t21, bar2) == 12);
static_assert(offsetof(struct t21, bar3) == 16);
static_assert(offsetof(struct t21, bar4) == 40);
static_assert(sizeof(struct t21) == 56);

// The rules also apply to anonymous bitfields with non-zero length.
struct t22
{
  char foo;
  short :2;
  char bar;
};
static_assert(alignof(struct t22) == 1);
static_assert(offsetof(struct t22, bar) == 2);

int main() {
  return 0;
}

