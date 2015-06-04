/* -*- C++ -*- */
namespace DebugCXX {
  struct Foo {
    int i;
    static int static_member;
  };
  enum Enum {
    Enumerator
  };
  template<typename T> struct traits {};
  template<typename T,
           typename Traits = traits<T>
          > class Bar {
    T value;
  };
}
namespace DebugCXX {
  extern template class Bar<int>;

  extern template struct traits<float>;
  typedef class Bar<float> FloatBar;

  inline void fn() {
    Bar<long> invisible;
  }
}
