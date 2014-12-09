__attribute__((objc_root_class))
@interface A
@end

struct X { };

void f1(int *x); // expected-warning{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}

typedef int *int_ptr;
typedef A *A_ptr;

#pragma clang assume_nonnull begin

void f2(int *x);
void f3(A* obj);
void f4(int (^block)(int, int));
void f5(int_ptr x);
void f6(A_ptr obj);
void f7(__nullable int *x);
void f8(__nullable A *obj);
void f9(int X::* mem_ptr);
void f10(int (X::*mem_func)(int, int));
void f11(__nullable int X::* mem_ptr);
void f12(__nullable int (X::*mem_func)(int, int));

int_ptr f13(void);
A *f14(void);

__null_unspecified int *f15(void);
__null_unspecified A *f16(void);

@interface A(Pragmas1)
- (A *)method1:(A_ptr)ptr;
- (null_unspecified A *)method2;

@property A *aProp;
@end

int *global_int_ptr;

typedef int *int_ptr_2;

static inline void f19(void) {
  float *fp = global_int_ptr; // expected-error{{cannot initialize a variable of type 'float *' with an lvalue of type '__nonnull int *'}}
}

#pragma clang assume_nonnull end

void f20(A *a); // expected-warning{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}
void f21(int_ptr x); // expected-warning{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}
void f22(A_ptr y); // expected-warning{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}
void f23(__nullable int_ptr x);
void f24(__nullable A_ptr y);
void f25(int_ptr_2 x);
