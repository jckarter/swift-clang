// RUN: %clang_cc1 -verify -Wno-deprecated-declarations -Wno-objc-root-class %s
// rdar://16076729

@partial_interface Unrelated;
@partial_interface NSObject;
@partial_interface NSString : NSObject;
@partial_interface NSArray : NSObject;
@partial_interface NSBlah ;
@partial_interface NSMutableArray : NSArray;
@partial_interface A;
@partial_interface B : A;
@partial_interface C : A;
@partial_interface D;
@partial_interface E;
@partial_interface F;
@partial_interface G ;
@partial_interface Fail;
@partial_interface WeirdNSString : NSString;
@partial_interface UIViewController : NSObject;


__attribute((objc_complete_definition))
@interface Unrelated
@end

__attribute((objc_complete_definition))
@interface NSObject
+ (id)new;
+ (id)alloc;
- (NSObject *)init;

- (id)retain;  // expected-note{{instance method 'retain' is assumed to return an instance of its receiver type ('NSArray *')}}
- autorelease;

- (id)self;

- (id)copy;
- (id)mutableCopy;

// Do not infer when instance/class mismatches
- (id)newNotInferred;
- (id)alloc;
+ (id)initWithBlarg;
+ (id)self;

// Do not infer when the return types mismatch.
- (Unrelated *)initAsUnrelated;
@end

__attribute((objc_complete_definition))
@interface NSString : NSObject
- (id)init;
- (id)initWithCString:(const char*)string;
@end

__attribute((objc_complete_definition))
@interface NSArray : NSObject
- (unsigned)count;
@end

__attribute((objc_complete_definition))
@interface NSBlah 
@end

__attribute((objc_complete_definition))
@interface NSMutableArray : NSArray
@end

@interface NSBlah ()
+ (Unrelated *)newUnrelated;
@end

void test_inference() {
  // Inference based on method family
  __typeof__(([[NSString alloc] init])) *str = (NSString**)0;
  __typeof__(([[[[NSString new] self] retain] autorelease])) *str2 = (NSString **)0;
  __typeof__(([[NSString alloc] initWithCString:"blah"])) *str3 = (NSString**)0;

  // Not inferred
  __typeof__(([[NSString new] copy])) *id1 = (id*)0;

  // Not inferred due to instance/class mismatches
  __typeof__(([[NSString new] newNotInferred])) *id2 = (id*)0;
  __typeof__(([[NSString new] alloc])) *id3 = (id*)0;
  __typeof__(([NSString self])) *id4 = (id*)0;
  __typeof__(([NSString initWithBlarg])) *id5 = (id*)0;

  // Not inferred due to return type mismatch
  __typeof__(([[NSString alloc] initAsUnrelated])) *unrelated = (Unrelated**)0;
  __typeof__(([NSBlah newUnrelated])) *unrelated2 = (Unrelated**)0;

  
  NSArray *arr = [[NSMutableArray alloc] init];
  NSMutableArray *marr = [arr retain]; // expected-warning{{incompatible pointer types initializing 'NSMutableArray *' with an expression of type 'NSArray *'}}
}

@implementation NSBlah
+ (Unrelated *)newUnrelated {
  return (Unrelated *)0;
}
@end

@implementation NSBlah (Cat)
+ (Unrelated *)newUnrelated2 {
  return (Unrelated *)0;
}
@end

__attribute((objc_complete_definition))
@interface A
- (id)initBlah; // expected-note 2{{overridden method is part of the 'init' method family}}
@end

__attribute((objc_complete_definition))
@interface B : A
- (Unrelated *)initBlah; // expected-warning{{method is expected to return an instance of its class type 'B', but is declared to return 'Unrelated *'}}
@end

__attribute((objc_complete_definition))
@interface C : A
@end

@implementation C
- (Unrelated *)initBlah {  // expected-warning{{method is expected to return an instance of its class type 'C', but is declared to return 'Unrelated *'}}
  return (Unrelated *)0;
}
@end

__attribute((objc_complete_definition))
@interface D
+ (id)newBlarg; // expected-note{{overridden method is part of the 'new' method family}}
@end

@interface D ()
+ alloc; // expected-note{{overridden method is part of the 'alloc' method family}}
@end

@implementation D
+ (Unrelated *)newBlarg { // expected-warning{{method is expected to return an instance of its class type 'D', but is declared to return 'Unrelated *'}}
  return (Unrelated *)0;
}

+ (Unrelated *)alloc { // expected-warning{{method is expected to return an instance of its class type 'D', but is declared to return 'Unrelated *'}}
  return (Unrelated *)0;
}
@end

@protocol P1
- (id)initBlah; // expected-note{{overridden method is part of the 'init' method family}}
- (int)initBlarg;
@end

@protocol P2 <P1>
- (int)initBlah; // expected-warning{{protocol method is expected to return an instance of the implementing class, but is declared to return 'int'}}
- (int)initBlarg;
- (int)initBlech;
@end

__attribute((objc_complete_definition))
@interface E
- init;
@end

@implementation E
- init {
  return self;
}
@end

@protocol P3
+ (NSString *)newString;
@end

__attribute((objc_complete_definition))
@interface F<P3>
@end

@implementation F
+ (NSString *)newString { return @"blah"; }
@end

// <rdar://problem/9340699>
__attribute((objc_complete_definition))
@interface G 
- (id)_ABC_init __attribute__((objc_method_family(init))); // expected-note {{method '_ABC_init' declared here}}
@end

@interface G (Additions)
- (id)_ABC_init2 __attribute__((objc_method_family(init)));
@end

@implementation G (Additions)
- (id)_ABC_init { // expected-warning {{category is implementing a method which will also be implemented by its primary class}}
  return 0;
}
- (id)_ABC_init2 {
  return 0;
}
- (id)_ABC_init3 {
  return 0;
}
@end

// PR12384
__attribute((objc_complete_definition))
@interface Fail @end
@protocol X @end
@implementation Fail
- (id<X>) initWithX // expected-note {{compiler has implicitly changed method 'initWithX' return type}}
{
  return (id)self; // expected-warning {{casting 'Fail *' to incompatible type 'id<X>'}}
}
@end

// <rdar://problem/11460990>

__attribute((objc_complete_definition))
@interface WeirdNSString : NSString
- (id)initWithCString:(const char*)string, void *blah;
@end


// rdar://14121570
@protocol PMFilterManager
@end

__attribute((objc_complete_definition))
@interface UIViewController : NSObject
@end

@implementation UIViewController
+ (UIViewController<PMFilterManager> *)newFilterViewControllerForType // expected-note {{compiler has implicitly changed method 'newFilterViewControllerForType' return type}}
{
        UIViewController<PMFilterManager> *filterVC;
        return filterVC; // expected-warning {{incompatible pointer types casting 'UIViewController *' to type 'UIViewController<PMFilterManager> *'}}
}
@end
