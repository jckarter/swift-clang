// RUN: %clang %xcode_cxx_warnings -c %s -O0 -g 

template <typename T>
class C {
  T data;

public:
  virtual T f0() { return data; }
};

int f0(C<int> x) {
  return x.f0();
}
