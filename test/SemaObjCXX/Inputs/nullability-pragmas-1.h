__attribute__((objc_root_class))
@interface NSError
@end

__attribute__((objc_root_class))
@interface A
@end

struct X { };

void f1(int *x); // expected-warning{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}

typedef struct __attribute__((objc_bridge(NSError))) __CFError *CFErrorRef;
typedef NSError *NSErrorPtr;
typedef NSError **NSErrorPtrPtr;
typedef CFErrorRef *CFErrorRefPtr;
typedef int *int_ptr;
typedef A *A_ptr;
typedef int (^block_ptr)(int, int);

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
void f12(int (X::* __nullable mem_func)(int, int));

int_ptr f13(void);
A *f14(void);

__null_unspecified int *f15(void);
__null_unspecified A *f16(void);
void f17(CFErrorRef *error); // expected-note{{no known conversion from '__nonnull A *' to '__nullable CFErrorRef * __nullable' (aka '__CFError **') for 1st argument}}
void f18(A **); // expected-warning 2{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}
void f19(CFErrorRefPtr error); // expected-warning{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}

void g1(int (^)(int, int));
void g2(int (^ *bp)(int, int)); // expected-warning{{block pointer is missing a nullability type specifier (__nonnull or __nullable)}}
// expected-warning@-1{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}
void g3(block_ptr *bp); // expected-warning{{block pointer is missing a nullability type specifier (__nonnull or __nullable)}}
// expected-warning@-1{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}
void g4(int (*fp)(int, int));
void g5(int (**fp)(int, int)); // expected-warning 2{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}

@interface A(Pragmas1)
+ (instancetype)aWithA:(A *)a;
- (A *)method1:(A_ptr)ptr;
- (null_unspecified A *)method2;
- (void)method3:(NSError **)error; // expected-note{{passing argument to parameter 'error' here}}
- (void)method4:(NSErrorPtr *)error; // expected-note{{passing argument to parameter 'error' here}}
- (void)method5:(NSErrorPtrPtr)error; // expected-warning{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}

@property A *aProp;
@property NSError **anError; // expected-warning 2{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}
@end

int *global_int_ptr;

// typedefs not inferred __nonnull
typedef int *int_ptr_2;

typedef int * // expected-warning{{pointer is missing a nullability type}}
            *int_ptr_ptr;

static inline void f30(void) {
  float *fp = global_int_ptr; // expected-error{{cannot initialize a variable of type 'float *' with an lvalue of type '__nonnull int *'}}

  int_ptr_2 ip2;
  float *fp2 = ip2; // expected-error{{cannot initialize a variable of type 'float *' with an lvalue of type 'int_ptr_2' (aka 'int *')}}

  int_ptr_ptr ipp;
  float *fp3 = ipp; // expected-error{{lvalue of type 'int_ptr_ptr' (aka 'int **')}}
}

#pragma clang assume_nonnull end

void f20(A *a); // expected-warning{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}
void f21(int_ptr x); // expected-warning{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}
void f22(A_ptr y); // expected-warning{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}
void f23(__nullable int_ptr x);
void f24(__nullable A_ptr y);
void f25(int_ptr_2 x); // expected-warning{{pointer is missing a nullability type specifier}}

@interface A(OutsidePragmas1)
+ (instancetype)aWithInt:(int)value; // expected-warning{{pointer is missing a nullability type specifier}}
@end
