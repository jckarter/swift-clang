/* -*- C++ -*- */
namespace DebugCXX {
  struct Foo {
    int i;
    static int static_member;
  };
  enum Enum {
    Enumerator
  };
  enum {
    e1 = '1'
  };
  enum {
    e2 = '2'
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

  // The follow snippet would not work with USRs because the forward
  // declaration gets a different USR than the partial specialization.
  template <typename...> class A;
  template <typename T> class A<T> {};
  typedef A<void> B;
  void foo(B) {}
}
