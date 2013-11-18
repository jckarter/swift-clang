// RUN: %clang_cc1 -std=c++98 %s -verify -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++11 %s -verify -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++1y %s -verify -fexceptions -fcxx-exceptions -pedantic-errors

#if __cplusplus < 201103L
#define fold(x) (__builtin_constant_p(x) ? (x) : (x))
#else
#define fold
#endif

namespace dr200 { // dr200: dup 214
  template <class T> T f(int);
  template <class T, class U> T f(U) = delete; // expected-error 0-1{{extension}}

  void g() {
    f<int>(1);
  }
}

// dr201 FIXME: write codegen test

namespace dr202 { // dr202: yes
  template<typename T> T f();
  template<int (*g)()> struct X {
    int arr[fold(g == &f<int>) ? 1 : -1];
  };
  template struct X<f>;
}

// FIXME (export) dr204: no

namespace dr206 { // dr206: yes
  struct S; // expected-note 2{{declaration}}
  template<typename T> struct Q { S s; }; // expected-error {{incomplete}}
  template<typename T> void f() { S s; } // expected-error {{incomplete}}
}

namespace dr207 { // dr207: yes
  class A {
  protected:
    static void f() {}
  };
  class B : A {
  public:
    using A::f;
    void g() {
      A::f();
      f();
    }
  };
}

// dr208 FIXME: write codegen test

namespace dr209 { // dr209: yes
  class A {
    void f(); // expected-note {{here}}
  };
  class B {
    friend void A::f(); // expected-error {{private}}
  };
}

// dr210 FIXME: write codegen test

namespace dr211 { // dr211: yes
  struct A {
    A() try {
      throw 0;
    } catch (...) {
      return; // expected-error {{return in the catch of a function try block of a constructor}}
    }
  };
}

namespace dr213 { // dr213: yes
  template <class T> struct A : T {
    void h(T t) {
      char &r1 = f(t);
      int &r2 = g(t); // expected-error {{undeclared}}
    }
  };
  struct B {
    int &f(B);
    int &g(B); // expected-note {{in dependent base class}}
  };
  char &f(B);

  template void A<B>::h(B); // expected-note {{instantiation}}
}

namespace dr214 { // dr214: yes
  template<typename T, typename U> T checked_cast(U from) { U::error; }
  template<typename T, typename U> T checked_cast(U *from);
  class C {};
  void foo(int *arg) { checked_cast<const C *>(arg); }

  template<typename T> T f(int);
  template<typename T, typename U> T f(U) { T::error; }
  void g() {
    f<int>(1);
  }
}

namespace dr215 { // dr215: yes
  template<typename T> class X {
    friend void T::foo();
    int n;
  };
  struct Y {
    void foo() { (void)+X<Y>().n; }
  };
}

namespace dr216 { // dr216: no
  // FIXME: Should reject this: 'f' has linkage but its type does not,
  // and 'f' is odr-used but not defined in this TU.
  typedef enum { e } *E;
  void f(E);
  void g(E e) { f(e); }

  struct S {
    // FIXME: Should reject this: 'f' has linkage but its type does not,
    // and 'f' is odr-used but not defined in this TU.
    typedef enum { e } *E;
    void f(E);
  };
  void g(S s, S::E e) { s.f(e); }
}

namespace dr217 { // dr217: yes
  template<typename T> struct S {
    void f(int);
  };
  template<typename T> void S<T>::f(int = 0) {} // expected-error {{default arguments cannot be added}}
}

namespace dr218 { // dr218: yes
  namespace A {
    struct S {};
    void f(S);
  }
  namespace B {
    struct S {};
    void f(S);
  }

  struct C {
    int f;
    void test1(A::S as) { f(as); } // expected-error {{called object type 'int'}}
    void test2(A::S as) { void f(); f(as); } // expected-error {{too many arguments}} expected-note {{}}
    void test3(A::S as) { using A::f; f(as); } // ok
    void test4(A::S as) { using B::f; f(as); } // ok
    void test5(A::S as) { int f; f(as); } // expected-error {{called object type 'int'}}
    void test6(A::S as) { struct f {}; (void) f(as); } // expected-error {{no matching conversion}} expected-note +{{}}
  };

  namespace D {
    struct S {};
    struct X { void operator()(S); } f;
  }
  void testD(D::S ds) { f(ds); } // expected-error {{undeclared identifier}}

  namespace E {
    struct S {};
    struct f { f(S); };
  }
  void testE(E::S es) { f(es); } // expected-error {{undeclared identifier}}

  namespace F {
    struct S {
      template<typename T> friend void f(S, T) {}
    };
  }
  void testF(F::S fs) { f(fs, 0); }

  namespace G {
    namespace X {
      int f;
      struct A {};
    }
    namespace Y {
      template<typename T> void f(T);
      struct B {};
    }
    template<typename A, typename B> struct C {};
  }
  void testG(G::C<G::X::A, G::Y::B> gc) { f(gc); }
}

// dr219: na
// dr220: na

namespace dr221 { // dr221: yes
  struct A {
    A &operator=(int&);
    A &operator+=(int&);
    static A &operator=(A&, double&); // expected-error {{cannot be a static member}}
    static A &operator+=(A&, double&); // expected-error {{cannot be a static member}}
    friend A &operator=(A&, char&); // expected-error {{must be a non-static member function}}
    friend A &operator+=(A&, char&);
  };
  A &operator=(A&, float&); // expected-error {{must be a non-static member function}}
  A &operator+=(A&, float&);

  void test(A a, int n, char c, float f) {
    a = n;
    a += n;
    a = c;
    a += c;
    a = f;
    a += f;
  }
}

// dr222 is a mystery -- it lists no changes to the standard, and yet was
// apparently both voted into the WP and acted upon by the editor.

// dr223: na

namespace dr224 { // dr224: no
  namespace example1 {
    template <class T> class A {
      typedef int type;
      A::type a;
      A<T>::type b;
      A<T*>::type c; // expected-error {{missing 'typename'}}
      ::dr224::example1::A<T>::type d;

      class B {
        typedef int type;

        A::type a;
        A<T>::type b;
        A<T*>::type c; // expected-error {{missing 'typename'}}
        ::dr224::example1::A<T>::type d;

        B::type e;
        A<T>::B::type f;
        A<T*>::B::type g; // expected-error {{missing 'typename'}}
        typename A<T*>::B::type h;
      };
    };

    template <class T> class A<T*> {
      typedef int type;
      A<T*>::type a;
      A<T>::type b; // expected-error {{missing 'typename'}}
    };

    template <class T1, class T2, int I> struct B {
      typedef int type;
      B<T1, T2, I>::type b1;
      B<T2, T1, I>::type b2; // expected-error {{missing 'typename'}}

      typedef T1 my_T1;
      static const int my_I = I;
      static const int my_I2 = I+0;
      static const int my_I3 = my_I;
      B<my_T1, T2, my_I>::type b3; // FIXME: expected-error {{missing 'typename'}}
      B<my_T1, T2, my_I2>::type b4; // expected-error {{missing 'typename'}}
      B<my_T1, T2, my_I3>::type b5; // FIXME: expected-error {{missing 'typename'}}
    };
  }

  namespace example2 {
    template <int, typename T> struct X { typedef T type; };
    template <class T> class A {
      static const int i = 5;
      X<i, int>::type w; // FIXME: expected-error {{missing 'typename'}}
      X<A::i, char>::type x; // FIXME: expected-error {{missing 'typename'}}
      X<A<T>::i, double>::type y; // FIXME: expected-error {{missing 'typename'}}
      X<A<T*>::i, long>::type z; // expected-error {{missing 'typename'}}
      int f();
    };
    template <class T> int A<T>::f() {
      return i;
    }
  }
}

// dr225: yes
template<typename T> void dr225_f(T t) { dr225_g(t); } // expected-error {{call to function 'dr225_g' that is neither visible in the template definition nor found by argument-dependent lookup}}
void dr225_g(int); // expected-note {{should be declared prior to the call site}}
template void dr225_f(int); // expected-note {{in instantiation of}}

namespace dr226 { // dr226: no
  template<typename T = void> void f() {}
#if __cplusplus < 201103L
  // expected-error@-2 {{extension}}
  // FIXME: This appears to be wrong: default arguments for function templates
  // are listed as a defect (in c++98) not an extension. EDG accepts them in
  // strict c++98 mode.
#endif
  template<typename T> struct S {
    template<typename U = void> void g();
#if __cplusplus < 201103L
  // expected-error@-2 {{extension}}
#endif
    template<typename U> struct X;
    template<typename U> void h();
  };
  template<typename T> template<typename U> void S<T>::g() {}
  template<typename T> template<typename U = void> struct S<T>::X {}; // expected-error {{cannot add a default template arg}}
  template<typename T> template<typename U = void> void S<T>::h() {} // expected-error {{cannot add a default template arg}}

  template<typename> void friend_h();
  struct A {
    // FIXME: This is ill-formed.
    template<typename=void> struct friend_B;
    // FIXME: f, h, and i are ill-formed.
    //  f is ill-formed because it is not a definition.
    //  h and i are ill-formed because they are not the only declarations of the
    //  function in the translation unit.
    template<typename=void> void friend_f();
    template<typename=void> void friend_g() {}
    template<typename=void> void friend_h() {}
    template<typename=void> void friend_i() {}
#if __cplusplus < 201103L
  // expected-error@-5 {{extension}} expected-error@-4 {{extension}}
  // expected-error@-4 {{extension}} expected-error@-3 {{extension}}
#endif
  };
  template<typename> void friend_i();

  template<typename=void, typename X> void foo(X) {}
  template<typename=void, typename X> struct Foo {}; // expected-error {{missing a default argument}} expected-note {{here}}
#if __cplusplus < 201103L
  // expected-error@-3 {{extension}}
#endif

  template<typename=void, typename X, typename, typename Y> int foo(X, Y);
  template<typename, typename X, typename=void, typename Y> int foo(X, Y);
  int x = foo(0, 0);
#if __cplusplus < 201103L
  // expected-error@-4 {{extension}}
  // expected-error@-4 {{extension}}
#endif
}

void dr227(bool b) { // dr227: yes
  if (b)
    int n;
  else
    int n;
}

namespace dr228 { // dr228: yes
  template <class T> struct X {
    void f();
  };
  template <class T> struct Y {
    void g(X<T> x) { x.template X<T>::f(); }
  };
}

namespace dr229 { // dr229: yes
  template<typename T> void f();
  template<typename T> void f<T*>() {} // expected-error {{function template partial specialization}}
  template<> void f<int>() {}
}

namespace dr231 { // dr231: yes
  namespace outer {
    namespace inner {
      int i; // expected-note {{here}}
    }
    void f() { using namespace inner; }
    int j = i; // expected-error {{undeclared identifier 'i'; did you mean 'inner::i'?}}
  }
}

// dr234: na
// dr235: na

namespace dr236 { // dr236: yes
  void *p = int();
#if __cplusplus < 201103L
  // expected-warning@-2 {{null pointer}}
#else
  // expected-error@-4 {{cannot initialize}}
#endif
}

namespace dr237 { // dr237: dup 470
  template<typename T> struct A { void f() { T::error; } };
  template<typename T> struct B : A<T> {};
  template struct B<int>; // ok
}

namespace dr239 { // dr239: yes
  namespace NS {
    class T {};
    void f(T);
    float &g(T, int);
  }
  NS::T parm;
  int &g(NS::T, float);
  int main() {
    f(parm);
    float &r = g(parm, 1);
    extern int &g(NS::T, float);
    int &s = g(parm, 1);
  }
}

// dr240: dup 616

namespace dr241 { // dr241: yes
  namespace A {
    struct B {};
    template <int X> void f(); // expected-note 2{{candidate}}
    template <int X> void g(B);
  }
  namespace C {
    template <class T> void f(T t); // expected-note 2{{candidate}}
    template <class T> void g(T t); // expected-note {{candidate}}
  }
  void h(A::B b) {
    f<3>(b); // expected-error {{undeclared identifier}}
    g<3>(b); // expected-error {{undeclared identifier}}
    A::f<3>(b); // expected-error {{no matching}}
    A::g<3>(b);
    C::f<3>(b); // expected-error {{no matching}}
    C::g<3>(b); // expected-error {{no matching}}
    using C::f;
    using C::g;
    f<3>(b); // expected-error {{no matching}}
    g<3>(b);
  }
}

namespace dr243 { // dr243: yes
  struct B;
  struct A {
    A(B); // expected-note {{candidate}}
  };
  struct B {
    operator A() = delete; // expected-error 0-1{{extension}} expected-note {{candidate}}
  } b;
  A a1(b);
  A a2 = b; // expected-error {{ambiguous}}
}

namespace dr244 { // dr244: no
  struct B {}; struct D : B {}; // expected-note {{here}}

  D D_object;
  typedef B B_alias;
  B* B_ptr = &D_object;

  void f() {
    D_object.~B(); // expected-error {{expression does not match the type}}
    D_object.B::~B();
    B_ptr->~B();
    B_ptr->~B_alias();
    B_ptr->B_alias::~B();
    // This is valid under DR244.
    B_ptr->B_alias::~B_alias(); // FIXME: expected-error {{expected the class name after '~' to name a destructor}}
    B_ptr->dr244::~B(); // expected-error {{refers to a member in namespace}}
    B_ptr->dr244::~B_alias(); // expected-error {{refers to a member in namespace}}
  }
}

namespace dr245 { // dr245: yes
  struct S {
    enum E {}; // expected-note {{here}}
    class E *p; // expected-error {{does not match previous declaration}}
  };
}

namespace dr246 { // dr246: yes
  struct S {
    S() try { // expected-note {{try block}}
      throw 0;
X: ;
    } catch (int) {
      goto X; // expected-error {{protected scope}}
    }
  };
}

namespace dr247 { // dr247: yes
  struct A {};
  struct B : A {
    void f();
    void f(int);
  };
  void (A::*f)() = (void (A::*)())&B::f;

  struct C {
    void f();
    void f(int);
  };
  struct D : C {};
  void (C::*g)() = &D::f;
  void (D::*h)() = &D::f;

  struct E {
    void f();
  };
  struct F : E {
    using E::f;
    void f(int);
  };
  void (F::*i)() = &F::f;
}

namespace dr248 { // dr248: yes c++11
  // FIXME: Should this also apply to c++98 mode? This was a DR against C++98.
  int \u040d\u040e = 0;
#if __cplusplus < 201103L
  // FIXME: expected-error@-2 {{expected ';'}}
#endif
}

namespace dr249 { // dr249: yes
  template<typename T> struct X { void f(); };
  template<typename T> void X<T>::f() {}
}

namespace dr250 { // dr250: yes
  typedef void (*FPtr)(double x[]);

  template<int I> void f(double x[]);
  FPtr fp = &f<3>;

  template<int I = 3> void g(double x[]); // expected-error 0-1{{extension}}
  FPtr gp = &g<>;
}

namespace dr252 { // dr252: yes
  struct A {
    void operator delete(void*); // expected-note {{found}}
  };
  struct B {
    void operator delete(void*); // expected-note {{found}}
  };
  struct C : A, B {
    virtual ~C();
  };
  C::~C() {} // expected-error {{'operator delete' found in multiple base classes}}

  struct D {
    void operator delete(void*, int); // expected-note {{here}}
    virtual ~D();
  };
  D::~D() {} // expected-error {{no suitable member 'operator delete'}}

  struct E {
    void operator delete(void*, int);
    void operator delete(void*) = delete; // expected-error 0-1{{extension}} expected-note {{here}}
    virtual ~E();
  };
  E::~E() {} // expected-error {{deleted}}

  struct F {
    // If both functions are available, the first one is a placement delete.
    void operator delete(void*, __SIZE_TYPE__);
    void operator delete(void*) = delete; // expected-error 0-1{{extension}} expected-note {{here}}
    virtual ~F();
  };
  F::~F() {} // expected-error {{deleted}}

  struct G {
    void operator delete(void*, __SIZE_TYPE__);
    virtual ~G();
  };
  G::~G() {}
}

namespace dr254 { // dr254: yes
  template<typename T> struct A {
    typedef typename T::type type; // ok even if this is a typedef-name, because
                                   // it's not an elaborated-type-specifier
    typedef struct T::type foo; // expected-error {{elaborated type refers to a typedef}}
  };
  struct B { struct type {}; };
  struct C { typedef struct {} type; }; // expected-note {{here}}
  A<B>::type n;
  A<C>::type n; // expected-note {{instantiation of}}
}

// dr256: dup 624

namespace dr257 { // dr257: yes
  struct A { A(int); }; // expected-note {{here}}
  struct B : virtual A {
    B() {}
    virtual void f() = 0;
  };
  struct C : B {
    C() {}
  };
  struct D : B {
    D() {} // expected-error {{must explicitly initialize the base class 'dr257::A'}}
    void f();
  };
}

namespace dr258 { // dr258: yes
  struct A {
    void f(const int);
    template<typename> void g(int);
    float &h() const;
  };
  struct B : A {
    using A::f;
    using A::g;
    using A::h;
    int &f(int);
    template<int> int &g(int); // expected-note {{candidate}}
    int &h();
  } b;
  int &w = b.f(0);
  int &x = b.g<int>(0); // expected-error {{no match}}
  int &y = b.h();
  float &z = const_cast<const B&>(b).h();

  struct C {
    virtual void f(const int) = 0;
  };
  struct D : C {
    void f(int);
  } d;

  struct E {
    virtual void f() = 0; // expected-note {{unimplemented}}
  };
  struct F : E {
    void f() const {}
  } f; // expected-error {{abstract}}
}

namespace dr259 { // dr259: yes c++11
  template<typename T> struct A {};
  template struct A<int>; // expected-note {{previous}}
  template struct A<int>; // expected-error {{duplicate explicit instantiation}}

  // FIXME: We only apply this DR in C++11 mode.
  template<> struct A<float>;
  template struct A<float>;
#if __cplusplus < 201103L
  // expected-error@-2 {{extension}} expected-note@-3 {{here}}
#endif

  template struct A<char>; // expected-note {{here}}
  template<> struct A<char>; // expected-error {{explicit specialization of 'dr259::A<char>' after instantiation}}

  template<> struct A<double>;
  template<> struct A<double>;
  template<> struct A<double> {}; // expected-note {{here}}
  template<> struct A<double> {}; // expected-error {{redefinition}}

  template<typename T> struct B; // expected-note {{here}}
  template struct B<int>; // expected-error {{undefined}}

  template<> struct B<float>;
  template struct B<float>;
#if __cplusplus < 201103L
  // expected-error@-2 {{extension}} expected-note@-3 {{here}}
#endif
}

namespace dr261 { // dr261: no
#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wused-but-marked-unused"

  // FIXME: This is ill-formed, with a diagnostic required, because operator new
  // and operator delete are inline and odr-used, but not defined in this
  // translation unit.
  // We're also missing the -Wused-but-marked-unused diagnostic here.
  struct A {
    inline void *operator new(__SIZE_TYPE__) __attribute__((unused));
    inline void operator delete(void*) __attribute__((unused));
    A() {}
  };

  // FIXME: These are ill-formed, with a required diagnostic, for the same
  // reason.
  struct B {
    inline void operator delete(void*) __attribute__((unused));
    ~B() {}
  };
  struct C {
    inline void operator delete(void*) __attribute__((unused));
    virtual ~C() {}
  };

  struct D {
    inline void operator delete(void*) __attribute__((unused));
  };
  void h() { C::operator delete(0); } // expected-warning {{marked unused but was used}}

#pragma clang diagnostic pop
}

namespace dr262 { // dr262: yes
  int f(int = 0, ...);
  int k = f();
  int l = f(0);
  int m = f(0, 0);
}

namespace dr263 { // dr263: yes
  struct X {};
  struct Y {
#if __cplusplus < 201103L
    friend X::X() throw();
    friend X::~X() throw();
#else
    friend constexpr X::X() noexcept;
    friend X::~X();
#endif
    Y::Y(); // expected-error {{extra qualification}}
    Y::~Y(); // expected-error {{extra qualification}}
  };
}

// dr265: dup 353
// dr266: na
// dr269: na
// dr270: na
