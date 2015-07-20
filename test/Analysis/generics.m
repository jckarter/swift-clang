// RUN: %clang_cc1 -analyze -analyzer-checker=core,alpha.osx.cocoa.ObjCGenerics -verify -Wno-objc-method-access %s

#if !__has_feature(objc_generics)
#  error Compiler does not support Objective-C generics?
#endif

#if !__has_feature(objc_generics_variance)
#  error Compiler does not support co- and contr-variance?
#endif

#define nil 0

@protocol NSObject
+ (id)alloc;
- (id)init;
@end

@protocol NSCopying
@end

__attribute__((objc_root_class))
@interface NSObject <NSObject>
@end

@interface NSString : NSObject <NSCopying>
@end

@interface NSMutableString : NSString
@end

@interface NSNumber : NSObject <NSCopying>
@end

@interface NSArray<__covariant T> : NSObject
+ (instancetype)arrayWithObjects:(const T [])objects count:(int)count;
+ (instancetype)getEmpty;
+ (NSArray<T> *)getEmpty2;
- (int)contains:(T)obj;
- (T)getObjAtIndex:(int)idx;
@end

@interface MutableArray<T> : NSArray<T>
- (int)addObject:(T)obj;
@end

@interface LegacyMutableArray : MutableArray
@end

@interface LegacySpecialMutableArray : LegacyMutableArray
@end

@interface BuggyMutableArray<T> : MutableArray
@end

@interface BuggySpecialMutableArray<T> : BuggyMutableArray<T>
@end

@interface MyMutableStringArray : MutableArray<NSString *>
@end

@interface ExceptionalArray<ExceptionType> : MutableArray<NSString *>
- (ExceptionType) getException;
@end

int getUnknown();
NSArray *getStuff();
NSArray *getTypedStuff() {
  NSArray<NSNumber *> *c = getStuff();
  return c;
}

void doStuff(NSArray<NSNumber *> *);
void withArrString(NSArray<NSString *> *);
void withArrMutableString(NSArray<NSMutableString *> *);
void withMutArrString(MutableArray<NSString *> *);
void withMutArrMutableString(MutableArray<NSMutableString *> *);

void test(NSArray *a, NSArray<NSString *> *b,
          NSArray<NSNumber *> *c) {
  a = b;
  c = a; // expected-warning  {{Incompatible}}
  [a contains: [[NSNumber alloc] init]]; // expected-warning {{Incompatible}}
  [a contains: [[NSString alloc] init]];
  doStuff(a); // expected-warning {{Incompatible}}
}

void test2() {
  NSArray<NSString *> *a = getTypedStuff(); // expected-warning {{Incompatible}}
}

void test3(NSArray *a, NSArray<NSString *> *b) {
  b = a;
  [a contains: [[NSNumber alloc] init]]; // expected-warning {{Incompatible}}
  [a contains: [[NSString alloc] init]];
  doStuff(a); // expected-warning {{Incompatible}}
}

void test4(id a, NSArray<NSString *> *b) {
  b = a;
  [a contains: [[NSNumber alloc] init]]; // expected-warning {{Incompatible}}
  [a contains: [[NSString alloc] init]];
  doStuff(a); // expected-warning {{Incompatible}}
}

void test5(id a, NSArray<NSString *> *b,
           NSArray<NSNumber *> *c) {
  a = b;
  c = a; // expected-warning {{Incompatible}}
  [a contains: [[NSNumber alloc] init]]; // expected-warning {{Incompatible}}
  [a contains: [[NSString alloc] init]];
  doStuff(a); // expected-warning {{Incompatible}}
}

void test6(MutableArray *m, MutableArray<NSString *> *a,
           MutableArray<NSMutableString *> *b) {
  if (getUnknown() == 5) {
    m = a;  
    [m contains: [[NSString alloc] init]];
  } else {
    m = b;
    [m contains: [[NSMutableString alloc] init]];
  }
  [m addObject: [[NSString alloc] init]]; // expected-warning {{Incompatible}}
  [m addObject: [[NSMutableString alloc] init]];
}

void test8(id a, MutableArray<NSString *> *b) {
  b = a;
  doStuff(a); // expected-warning {{Incompatible}}
}

void test9(MutableArray *a,
           MutableArray<NSMutableString *> *b) {
  b = (MutableArray<NSMutableString *> *)a;
  [a addObject: [[NSString alloc] init]]; // expected-warning {{Incompatible}}
}

void test10(id d, MyMutableStringArray *a,
            MutableArray<NSString *> *b,
            MutableArray<NSNumber *> *c) {
  d = a;
  b = d;
  c = d; // expected-warning {{Incompatible}}
}

void test11(id d, ExceptionalArray<NSString *> *a,
            MutableArray<NSString *> *b,
            MutableArray<NSNumber *> *c) {
  d = a;
  [d contains: [[NSString alloc] init]];
  [d contains: [[NSNumber alloc] init]]; // expected-warning {{Incompatible}}
  b = d;
  c = d; // expected-warning {{Incompatible}}
}

void test12(id d, ExceptionalArray<NSString *> *a,
            MutableArray<NSString *> *b,
            MutableArray<NSNumber *> *c) {
  a = d;
  [d contains: [[NSString alloc] init]];
  [d contains: [[NSNumber alloc] init]]; // expected-warning {{Incompatible}}
  b = d;
  c = d; // expected-warning {{Incompatible}}
}

void test13(id a) {
  withMutArrString(a);
  withMutArrMutableString(a); // expected-warning {{Incompatible}}
}

void test14(id a) {
  withMutArrMutableString(a);
  withMutArrString(a); // expected-warning {{Incompatible}}
}

void test15(LegacyMutableArray *a) {
  withMutArrMutableString(a);
  withMutArrString(a); // expected-warning {{Incompatible}}
}

void test16(LegacySpecialMutableArray *a) {
  withMutArrString(a);
  withMutArrMutableString(a); // expected-warning {{Incompatible}}
}

void test17(BuggyMutableArray<NSMutableString *> *a) {
  withMutArrString(a);
  withMutArrMutableString(a); // expected-warning {{Incompatible}}
}

void test18(BuggySpecialMutableArray<NSMutableString *> *a) {
  withMutArrMutableString(a);
  withMutArrString(a); // expected-warning {{Incompatible}}
}

NSArray<NSString *> *getStrings();
void test19(NSArray<NSNumber *> *a) {
  NSArray *b = a;
  // Valid uses of NSArray of NSNumbers.
  b = getStrings();
  // Valid uses of NSArray of NSStrings.
}

void test20(NSArray<NSNumber *> *a) {
  NSArray *b = a;
  NSString *str = [b getObjAtIndex: 0]; // expected-warning {{Incompatible}}
  NSNumber *num = [b getObjAtIndex: 0];
}

void test21(id m, NSArray<NSMutableString *> *a,
            MutableArray<NSMutableString *> *b) {
  a = b;
  if (getUnknown() == 5) {
    m = a;  
    [m addObject: [[NSString alloc] init]]; // expected-warning {{Incompatible}}
  } else {
    m = b;
    [m addObject: [[NSMutableString alloc] init]];
  }
}

void test22(__kindof NSArray<NSString *> *a,
            MutableArray<NSMutableString *> *b) {
  a = b;
  if (getUnknown() == 5) {
    [a addObject: [[NSString alloc] init]]; // expected-warning {{Incompatible}}
  } else {
    [a addObject: [[NSMutableString alloc] init]];
  }
}

void test23() {
  // ObjCArrayLiterals are not specialized in the AST. 
  NSArray *arr = @[@"A", @"B"];
  [arr contains: [[NSNumber alloc] init]];
}

void test24() {
  NSArray<NSString *> *arr = @[@"A", @"B"];
  NSArray *arr2 = arr;
  [arr2 contains: [[NSNumber alloc] init]]; // expected-warning {{Incompatible}}
}

void test25(id a, MutableArray<NSMutableString *> *b) {
  a = b;
  [a nonExistentMethod];
}

void test26() {
  Class c = [NSArray<NSString *> class];
  NSArray<NSNumber *> *a = [c getEmpty]; // expected-warning {{Incompatible}}
  a = [c getEmpty2]; // expected-warning {{Incompatible}}
}
