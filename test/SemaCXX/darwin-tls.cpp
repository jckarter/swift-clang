// RUN: %clang_cc1 -triple x86_64-apple-macosx10.8 -std=c++11 -verify %s
// RUN: %clang_cc1 -triple thumbv7s-apple-ios8.0 -std=c++11 -verify %s
// RUN: %clang_cc1 -triple arm64-apple-ios7.1 -std=c++11 -verify %s

struct A {
  ~A();
};

thread_local A a; // expected-error{{thread-local storage is not supported for the current target}}
