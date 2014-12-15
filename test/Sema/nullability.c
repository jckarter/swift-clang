// RUN: %clang_cc1 -fsyntax-only -fblocks %s -verify

#if __has_feature(nullability)
#else
#  error nullability feature should be defined
#endif

typedef int * int_ptr;

// Parse nullability type specifiers.
typedef int * __nonnull nonnull_int_ptr;
typedef int * __nullable nullable_int_ptr;

// Redundant nullability type specifiers.
typedef int * __nonnull __nonnull redundant_1; // expected-warning{{duplicate nullability specifier '__nonnull'}}

// Conflicting nullability type specifiers.
typedef int * __nonnull __nullable conflicting_1; // expected-error{{nullability specifier '__nonnull' conflicts with existing specifier '__nullable'}}

// Redundant/conflicting nullability specifiers via a typedef are okay.
// FIXME: Test that these have proper override semantics.
typedef nonnull_int_ptr __nonnull redundant_okay_1;

// Nullability applies to all pointer types.
typedef int (* __nonnull function_pointer_type_1)(int, int);
typedef int (^ __nonnull block_type_1)(int, int);

// Nullability must be on a pointer type.
typedef int __nonnull int_type_1; // expected-error{{nullability specifier '__nonnull' cannot be applied to non-pointer type 'int'}}

// Nullability can move out to a pointer/block pointer declarator.
typedef __nonnull int * nonnull_int_ptr_2;
typedef int __nullable * nullable_int_ptr_2;
typedef __nonnull int (* function_pointer_type_2)(int, int);
typedef __nonnull int (^ block_type_2)(int, int);
typedef __nonnull int * * __nullable nonnull_int_ptr_ptr_1;

// Moving nullability where it creates a conflict.
typedef __nonnull int * __nullable *  conflict_int_ptr_ptr_2; // expected-error{{nullability specifier '__nullable' conflicts with existing specifier '__nonnull'}}

// Nullability is not part of the canonical type.
typedef int * __nonnull ambiguous_int_ptr;
typedef int * ambiguous_int_ptr;
typedef int * __nullable ambiguous_int_ptr;

// Printing of nullability.
float f;
int * __nonnull ip_1 = &f; // expected-warning{{incompatible pointer types initializing '__nonnull int *' with an expression of type 'float *'}}

// Check passing null to a __nonnull argument.
void accepts_nonnull_1(__nonnull int *ptr);
void (*accepts_nonnull_2)(__nonnull int *ptr);
void (^accepts_nonnull_3)(__nonnull int *ptr);

void test_accepts_nonnull_null_pointer_literal() {
  accepts_nonnull_1(0); // expected-warning{{null passed to a callee that requires a non-null argument}}
  accepts_nonnull_2(0); // expected-warning{{null passed to a callee that requires a non-null argument}}
  accepts_nonnull_3(0); // expected-warning{{null passed to a callee that requires a non-null argument}}
}

// Check returning nil from a __nonnull-returning function.
__nonnull int *returns_int_ptr(int x) {
  if (x) {
    return 0; // expected-warning{{null returned from function that requires a non-null return value}}
  }

  return (__nonnull int *)0;
}

// Check printing of nullability specifiers.
void printing_nullability(void) {
  int * __nonnull iptr;
  float *fptr = iptr; // expected-warning{{incompatible pointer types initializing 'float *' with an expression of type '__nonnull int *'}}

  int * * __nonnull iptrptr;
  float **fptrptr = iptrptr; // expected-warning{{incompatible pointer types initializing 'float **' with an expression of type 'int ** __nonnull'}}

  int * __nullable * __nonnull iptrptr2;
  float * *fptrptr2 = iptrptr2; // expected-warning{{incompatible pointer types initializing 'float **' with an expression of type '__nullable int ** __nonnull'}}
}

// Check nullable-to-nonnull conversions.
void nullable_to_nonnull(__nullable int *ptr) {
  int *a = ptr; // okay
  __nonnull int *b = ptr; // expected-warning{{implicit conversion from nullable pointer '__nullable int *' to non-nullable pointer type '__nonnull int *'}}
}