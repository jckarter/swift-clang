// RUN: %clang_cc1 -fsyntax-only -Woverriding-method-mismatch %s -verify

__attribute__((objc_root_class))
@interface NSFoo
- (void)methodTakingIntPtr:(__nonnull int *)ptr;
- (__nonnull int *)methodReturningIntPtr;
@end

// Nullability applies to all pointer types.
typedef NSFoo * __nonnull nonnull_NSFoo_ptr;
typedef id __nonnull nonnull_id;
typedef SEL __nonnull nonnull_SEL;

// Nullability can move into Objective-C pointer types.
typedef __nonnull NSFoo * nonnull_NSFoo_ptr_2;

// Conflicts from nullability moving into Objective-C pointer type.
typedef __nonnull NSFoo * __nullable conflict_NSFoo_ptr_2; // expected-error{{nullability specifier '__nullable' conflicts with existing specifier '__nonnull'}}

// Check returning nil from a __nonnull-returning method.
@implementation NSFoo
- (void)methodTakingIntPtr:(__nonnull int *)ptr { }
- (__nonnull int *)methodReturningIntPtr {
  return 0; // expected-warning{{null returned from method that requires a non-null return value}}
}
@end

// Context-sensitive keywords and property attributes for nullability.
__attribute__((objc_root_class))
@interface NSBar
- (nonnull NSFoo *)methodWithFoo:(nonnull NSFoo *)foo;

- (nonnull NSFoo **)invalidMethod1; // expected-error{{nullability keyword 'nonnull' cannot be applied to multi-level pointer type 'NSFoo **'}}
// expected-note@-1{{use nullability type specifier '__nonnull' to affect the innermost pointer type of 'NSFoo **'}}
- (nonnull NSFoo * __nullable)conflictingMethod1; // expected-error{{nullability specifier '__nullable' conflicts with existing specifier '__nonnull'}}
- (nonnull NSFoo * __nonnull)redundantMethod1; // expected-warning{{duplicate nullability specifier '__nonnull'}}

@property(nonnull,retain) NSFoo *property1;
@property(nullable,assign) NSFoo ** invalidProperty1; // expected-error{{nullability keyword 'nullable' cannot be applied to multi-level pointer type 'NSFoo **'}}
// expected-note@-1{{use nullability type specifier '__nullable' to affect the innermost pointer type of 'NSFoo **'}}
@property(null_unspecified,retain) NSFoo __nullable *conflictingProperty1; // expected-error{{nullability specifier 'null_unspecified' conflicts with existing specifier '__nullable'}}
@property(retain,nonnull) NSFoo * __nonnull redundantProperty1; // expected-warning{{duplicate nullability specifier '__nonnull'}}

@property(null_unspecified,retain,nullable) NSFoo *conflictingProperty3; // expected-error{{nullability specifier 'nullable' conflicts with existing specifier 'null_unspecified'}}
@property(nullable,retain,nullable) NSFoo *redundantProperty3; // expected-warning{{duplicate nullability specifier 'nullable'}}
@end

@interface NSBar ()
@property(nonnull,retain) NSFoo *property2;
@property(nullable,assign) NSFoo ** invalidProperty2; // expected-error{{nullability keyword 'nullable' cannot be applied to multi-level pointer type 'NSFoo **'}}
// expected-note@-1{{use nullability type specifier '__nullable' to affect the innermost pointer type of 'NSFoo **'}}
@property(null_unspecified,retain) NSFoo __nullable *conflictingProperty2; // expected-error{{nullability specifier 'null_unspecified' conflicts with existing specifier '__nullable'}}
@property(retain,nonnull) NSFoo * __nonnull redundantProperty2; // expected-warning{{duplicate nullability specifier '__nonnull'}}
@end

void test_accepts_nonnull_null_pointer_literal(NSFoo *foo, __nonnull NSBar *bar) {
  [foo methodTakingIntPtr: 0]; // expected-warning{{null passed to a callee that requires a non-null argument}}
  [bar methodWithFoo: 0]; // expected-warning{{null passed to a callee that requires a non-null argument}}
  bar.property1 = 0; // expected-warning{{null passed to a callee that requires a non-null argument}}
  bar.property2 = 0; // expected-warning{{null passed to a callee that requires a non-null argument}}
  [bar setProperty1: 0]; // expected-warning{{null passed to a callee that requires a non-null argument}}
  [bar setProperty2: 0]; // expected-warning{{null passed to a callee that requires a non-null argument}}
  int *ptr = bar.property1; // expected-warning{{incompatible pointer types initializing 'int *' with an expression of type '__nonnull NSFoo *'}}
}

// Check returning nil from a nonnull-returning method.
@implementation NSBar
- (nonnull NSFoo *)methodWithFoo:(nonnull NSFoo *)foo {
  return 0; // expected-warning{{null returned from method that requires a non-null return value}}
}

- (NSFoo **)invalidMethod1 { 
  return 0; 
}

- (NSFoo *)conflictingMethod1 { 
  return 0; // expected-warning{{null returned from method that requires a non-null return value}}
}
- (NSFoo *)redundantMethod1 {
  return 0; // expected-warning{{null returned from method that requires a non-null return value}}
}
@end

__attribute__((objc_root_class))
@interface NSMerge
- (nonnull NSFoo *)methodA:(nonnull NSFoo*)foo;
- (nonnull NSFoo *)methodB:(nonnull NSFoo*)foo;
- (NSFoo *)methodC:(NSFoo*)foo;
@end

@implementation NSMerge
- (NSFoo *)methodA:(NSFoo*)foo {
  int *ptr = foo; // expected-warning{{incompatible pointer types initializing 'int *' with an expression of type '__nonnull NSFoo *'}}
  return 0; // expected-warning{{null returned from method that requires a non-null return value}}
}

- (nullable NSFoo *)methodB:(null_unspecified NSFoo*)foo { // expected-error{{nullability specifier 'nullable' conflicts with existing specifier 'nonnull'}} \
  // expected-error{{nullability specifier 'null_unspecified' conflicts with existing specifier 'nonnull'}}
  return 0;
}

- (nonnull NSFoo *)methodC:(nullable NSFoo*)foo {
  return 0; // expected-warning{{null returned from method that requires a non-null return value}}
}
@end

// Checking merging of nullability when sending a message.
@interface NSMergeReceiver
- (id)returnsNone;
- (nonnull id)returnsNonNull;
- (nullable id)returnsNullable;
- (null_unspecified id)returnsNullUnspecified;
@end

void test_receiver_merge(NSMergeReceiver *none,
                         __nonnull NSMergeReceiver *nonnull,
                         __nullable NSMergeReceiver *nullable,
                         __null_unspecified NSMergeReceiver *null_unspecified) {
  int *ptr;

  ptr = [nullable returnsNullable]; // expected-warning{{'__nullable id'}}
  ptr = [nullable returnsNullUnspecified]; // expected-warning{{'__nullable id'}}
  ptr = [nullable returnsNonNull]; // expected-warning{{'__nullable id'}}
  ptr = [nullable returnsNone]; // expected-warning{{'__nullable id'}}

  ptr = [null_unspecified returnsNullable]; // expected-warning{{'__nullable id'}}
  ptr = [null_unspecified returnsNullUnspecified]; // expected-warning{{'__null_unspecified id'}}
  ptr = [null_unspecified returnsNonNull]; // expected-warning{{'__null_unspecified id'}}
  ptr = [null_unspecified returnsNone]; // expected-warning{{'id'}}

  ptr = [nonnull returnsNullable]; // expected-warning{{'__nullable id'}}
  ptr = [nonnull returnsNullUnspecified]; // expected-warning{{'__null_unspecified id'}}
  ptr = [nonnull returnsNonNull]; // expected-warning{{'__nonnull id'}}
  ptr = [nonnull returnsNone]; // expected-warning{{'id'}}

  ptr = [none returnsNullable]; // expected-warning{{'__nullable id'}}
  ptr = [none returnsNullUnspecified]; // expected-warning{{'id'}}
  ptr = [none returnsNonNull]; // expected-warning{{'id'}}
  ptr = [none returnsNone]; // expected-warning{{'id'}}
  
}

@protocol Initializable
- (instancetype)initWithBlah:(id)blah;
@end

__attribute__((objc_root_class))
@interface InitializableClass <Initializable>
- (nonnull instancetype)initWithBlah:(nonnull)blah;
@end
