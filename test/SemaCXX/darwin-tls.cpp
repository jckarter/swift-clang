// RUN: %clang_cc1 -triple x86_64-apple-macosx10.8 -std=c++11 -verify %s

struct A {
  ~A();
};

thread_local A a; // expected-error{{thread-local storage is unsupported for the current target}}
