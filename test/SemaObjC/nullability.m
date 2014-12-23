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
  return 0; // no warning
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
@property(null_unspecified,assign) NSFoo ** invalidProperty3; // expected-error{{nullability keyword 'null_unspecified' cannot be applied to multi-level pointer type 'NSFoo **'}}
@property(null_unspecified,assign) int invalidProperty4; // expected-error{{nullability specifier 'null_unspecified' cannot be applied to non-pointer type 'int'}}
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

@implementation NSBar
// Check returning nil from a nonnull-returning method.
- (nonnull NSFoo *)methodWithFoo:(nonnull NSFoo *)foo {
  return 0; // no warning
}

- (NSFoo **)invalidMethod1 { 
  return 0; 
}

- (NSFoo *)conflictingMethod1 { 
  return 0; // no warning
}
- (NSFoo *)redundantMethod1 {
  return 0; // no warning
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
  return 0; // no warning
}

- (nullable NSFoo *)methodB:(nullable NSFoo*)foo { // expected-error{{nullability specifier 'nullable' conflicts with existing specifier 'nonnull'}} \
  // expected-error{{nullability specifier 'nullable' conflicts with existing specifier 'nonnull'}}
  return 0;
}

- (nonnull NSFoo *)methodC:(nullable NSFoo*)foo {
  return 0; // no warning
}
@end

// Checking merging of nullability when sending a message.
@interface NSMergeReceiver
- (id)returnsNone;
- (nonnull id)returnsNonNull;
- (nullable id)returnsNullable;
@end

void test_receiver_merge(NSMergeReceiver *none,
                         __nonnull NSMergeReceiver *nonnull,
                         __nullable NSMergeReceiver *nullable) {
  int *ptr;

  ptr = [nullable returnsNullable]; // expected-warning{{'__nullable id'}}
  ptr = [nullable returnsNonNull]; // expected-warning{{'__nullable id'}}
  ptr = [nullable returnsNone]; // expected-warning{{'__nullable id'}}

  ptr = [nonnull returnsNullable]; // expected-warning{{'__nullable id'}}
  ptr = [nonnull returnsNonNull]; // expected-warning{{'__nonnull id'}}
  ptr = [nonnull returnsNone]; // expected-warning{{'id'}}

  ptr = [none returnsNullable]; // expected-warning{{'__nullable id'}}
  ptr = [none returnsNonNull]; // expected-warning{{'id'}}
  ptr = [none returnsNone]; // expected-warning{{'id'}}
  
}

// instancetype
@protocol Initializable
- (instancetype)initWithBlah:(id)blah;
@end

__attribute__((objc_root_class))
@interface InitializableClass <Initializable>
- (nonnull instancetype)initWithBlah:(nonnull id)blah;
- (nullable instancetype)returnMe;
+ (nullable instancetype)returnInstanceOfMe;
@end

void test_instancetype(__nonnull InitializableClass *ic, __nonnull id object) {
  int *ip = [ic returnMe]; // expected-warning{{incompatible pointer types initializing 'int *' with an expression of type '__nullable InitializableClass *'}}
  ip = [InitializableClass returnMe]; // expected-warning{{incompatible pointer types assigning to 'int *' from '__nullable id'}}
  ip = [InitializableClass returnInstanceOfMe]; // expected-warning{{incompatible pointer types assigning to 'int *' from '__nullable InitializableClass *'}}
  ip = [object returnMe]; // expected-warning{{incompatible pointer types assigning to 'int *' from '__nullable id'}}
}

// Check null_resettable getters/setters.
__attribute__((objc_root_class))
@interface NSResettable
@property(null_resettable,retain) NSResettable *resettable1; // expected-note{{passing argument to parameter 'resettable1' here}}
@property(null_resettable,retain,nonatomic) NSResettable *resettable2;
@property(null_resettable,retain,nonatomic) NSResettable *resettable3;
@property(null_resettable,retain,nonatomic) NSResettable *resettable4;
@property(null_resettable,retain,nonatomic) NSResettable *resettable5;
@property(null_resettable,retain,nonatomic) NSResettable *resettable6;
@end

void test_null_resettable(NSResettable *r, int *ip) {
  [r setResettable1:ip]; // expected-warning{{incompatible pointer types sending 'int *' to parameter of type '__nullable NSResettable *'}}
  r.resettable1 = ip; // expected-warning{{incompatible pointer types assigning to '__nullable NSResettable *' from 'int *'}}
}

@implementation NSResettable // expected-warning{{synthesized setter 'setResettable4:' for null_resettable property 'resettable4' does not handle nil}}
- (NSResettable *)resettable1 {
  return 0; // no warning
}

- (void)setResettable1:(NSResettable *)param {
}

@synthesize resettable2; // no warning; not synthesized
@synthesize resettable3; // expected-warning{{synthesized setter 'setResettable3:' for null_resettable property 'resettable3' does not handle nil}}

- (void)setResettable2:(NSResettable *)param {
}

@dynamic resettable5;

- (NSResettable *)resettable6 {
  return 0; // no warning
}
@end
