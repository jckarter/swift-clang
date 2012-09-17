// RUN: %clang_cc1 -fsyntax-only -verify %s

// PR4103 : Make sure we don't get a bogus unused expression warning
namespace PR4103 {
  class APInt {
    char foo;
  };
  class APSInt : public APInt {
    char bar;
  public:
    APSInt &operator=(const APSInt &RHS);
  };

  APSInt& APSInt::operator=(const APSInt &RHS) {
    APInt::operator=(RHS);
    return *this;
  }

  template<typename T>
  struct X {
    X();
  };

  void test() {
    X<int>();
  }
}

namespace derefvolatile {
  void f(volatile char* x) {
    *x; // expected-warning {{expression result would be unused in standard C++98}}
    (void)*x; // expected-warning {{expression result would be unused in standard C++98}}
    volatile char y = 10;
    (void)y; // don't warn here, because it's a common pattern.
  }
}
