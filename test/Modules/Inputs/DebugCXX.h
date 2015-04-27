/* -*- C++ -*- */
namespace DebugCXX {
  struct Foo {
    int i;
    static int static_member;
  };
  enum Enum {
    Enumerator
  };
  template<typename T> struct traits;
  template<typename T,
           typename Traits = traits<T>
          > class Bar {
    T value;
  };
  void bar(Bar<int> &);
}
namespace DebugCXX {
  template class Bar<int>;
}
