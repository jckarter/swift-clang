// RUN: %clang_cc1 -fsyntax-only %s -verify

@interface NSFoo
- (void)methodTakingIntPtr:(__nonnull int *)ptr;
@end

// Nullability applies to all pointer types.
typedef NSFoo * __nonnull nonnull_NSFoo_ptr;
typedef id __nonnull nonnull_id;
typedef SEL __nonnull nonnull_SEL;

// Nullability can move into Objective-C pointer types.
typedef __nonnull NSFoo * nonnull_NSFoo_ptr_2;

// Conflicts from nullability moving into Objective-C pointer type.
typedef __nonnull NSFoo * __nullable conflict_NSFoo_ptr_2; // expected-error{{nullability specifier '__nullable' conflicts with existing specifier '__nonnull'}}

void test_accepts_nonnull_null_pointer_literal(NSFoo *foo) {
  [foo methodTakingIntPtr: 0]; // expected-warning{{null passed to a callee that requires a non-null argument}}
}

