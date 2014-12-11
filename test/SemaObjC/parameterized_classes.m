// RUN: %clang_cc1 %s -verify

@protocol NSObject
@end

__attribute__((objc_root_class))
@interface NSObject <NSObject>
@end

// --------------------------------------------------------------------------
// Parsing parameterized classes.
// --------------------------------------------------------------------------

// Parse type parameters with a bound
@interface PC1<T : id, U : NSObject*> : NSObject 
@end

// Parse a type parameter with a bound that terminates in '>>'.
@interface PC2<T : id<NSObject>> : NSObject
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
@interface PC6<T, U, T> : NSObject // FIXME: ERROR HERE
@end

// Parse Objective-C protocol references.
@interface PC7<T> // expected-error{{cannot find protocol declaration for 'T'}}
@end

// Parse both type parameters and protocol references.
@interface PC8<T> : NSObject <NSObject>
@end

// --------------------------------------------------------------------------
// Parsing parameterized categories and extensions.
// --------------------------------------------------------------------------
@interface PC1<T> (Cat1) <NSObject>
@end

@interface PC1<T : id> (Cat2) <NSObject>
@end

@interface PC1<T> () <NSObject>
@end

@interface PC1<T : id> () <NSObject>
@end

// FIXME: Type parameter mismatches between categories/extensions and
// @interface.

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
