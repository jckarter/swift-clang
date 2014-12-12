// RUN: %clang_cc1 %s -verify

@protocol NSObject
@end

__attribute__((objc_root_class))
@interface NSObject <NSObject>
@end

@interface NSString : NSObject
@end

// --------------------------------------------------------------------------
// Parsing parameterized classes.
// --------------------------------------------------------------------------

// Parse type parameters with a bound
@interface PC1<T, U : NSObject*> : NSObject
// expected-note@-1{{type parameter 'T' declared here}}
// expected-note@-2{{type parameter 'U' declared here}}
@end

// Parse a type parameter with a bound that terminates in '>>'.
@interface PC2<T : id<NSObject>> : NSObject // expected-error{{a space is required between consecutive right angle brackets (use '> >')}}
@end

// Parse multiple type parameters.
@interface PC3<T, U : id> : NSObject
@end

// Parse multiple type parameters--grammatically ambiguous with protocol refs.
@interface PC4<T, U, V> : NSObject
@end

// Parse a type parameter list without a superclass.
@interface PC5<T : id> // expected-error{{parameterized Objective-C class 'PC5' must have a superclass}}
@end

// Parse a type parameter with name conflicts.
@interface PC6<T, U, 
               T> : NSObject // expected-error{{redeclaration of type parameter 'T'}}
@end

// Parse Objective-C protocol references.
@interface PC7<T> // expected-error{{cannot find protocol declaration for 'T'}}
@end

// Parse both type parameters and protocol references.
@interface PC8<T> : NSObject <NSObject>
@end

// Type parameters with improper bounds.
@interface PC9<T : int, // expected-error{{type bound 'int' for type parameter 'T' is not an Objective-C pointer type}}
               U : NSString> : NSObject // expected-error{{missing '*' in type bound 'NSString' for type parameter 'U'}}
@end

// --------------------------------------------------------------------------
// Parsing parameterized categories and extensions.
// --------------------------------------------------------------------------

// Inferring type bounds
@interface PC1<T, U> (Cat1) <NSObject>
@end

// Matching type bounds
@interface PC1<T : id, U : NSObject *> (Cat2) <NSObject>
@end

// Inferring type bounds
@interface PC1<T, U> () <NSObject>
@end

// Matching type bounds
@interface PC1<T : id, U : NSObject *> () <NSObject>
@end

// Missing type parameters.
@interface PC1<T> () // expected-error{{extension has too few type parameters (expected 2, have 1)}}
@end

// Extra type parameters.
@interface PC1<T, U, V> (Cat3) // expected-error{{category has too many type parameters (expected 2, have 3)}}
@end

// Mismatched bounds.
@interface PC1<T : NSObject *, // expected-error{{type bound 'NSObject *' for type parameter 'T' conflicts with implicit bound 'id'}}
               X : id> () // expected-error{{type bound 'id' for type parameter 'X' conflicts with previous bound 'NSObject *'for type parameter 'U'}}
@end

// Parameterized category/extension of non-parameterized class.
@interface NSObject<T> (Cat1) // expected-error{{category of non-parameterized class 'NSObject' cannot have type parameters}}
@end

@interface NSObject<T> () // expected-error{{extension of non-parameterized class 'NSObject' cannot have type parameters}}
@end

// --------------------------------------------------------------------------
// @implementations cannot have type parameters
// --------------------------------------------------------------------------
@implementation PC1<T : id> // expected-error{{@implementation cannot have type parameters}}
@end

@implementation PC2<T> // expected-error{{@implementation declaration cannot be protocol qualified}}
@end

@implementation PC1<T> (Cat1) // expected-error{{@implementation cannot have type parameters}}
@end

@implementation PC1<T : id> (Cat2) // expected-error{{@implementation cannot have type parameters}}
@end
