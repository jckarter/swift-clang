/* -*- C++ -*- */
namespace DebugCXX {
  struct Foo {
    int i;
    static int static_member;
  };
  enum Enum {
    Enumerator
  };
  template<typename T> class Bar {
    T value;
  };
}
