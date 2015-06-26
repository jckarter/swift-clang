// RUN: %clang_cc1 -analyze -analyzer-checker=core,alpha.osx.cocoa.ObjCGenerics -verify %s

#if !__has_feature(objc_generics)
#  error Compiler does not support Objective-C generics?
#endif

#if !__has_feature(objc_generics_variance)
#  error Compiler does not support co- and contr-variance?
#endif

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

@interface Array<__covariant T> : NSObject
- (int)contains:(T)obj;
@end

@interface MutableArray<T> : Array<T>
@end

@interface LegacyArray : Array
@end

@interface LegacyMutableArray : LegacyArray
@end

@interface BuggyArray<T> : Array
@end

@interface BuggyMutableArray<T> : BuggyArray<T>
@end

@interface MyMutableStringArray : MutableArray<NSString *>
@end

@interface ExceptionalArray<ExceptionType> : MutableArray<NSString *>
- (ExceptionType) getException;
@end

int getUnknown();
Array *getStuff();
Array *getTypedStuff() {
  Array<NSNumber *> *c = getStuff();
  return c;
}

void doStuff(Array<NSNumber *> *);
void withString(Array<NSString *> *);
void withMutableString(Array<NSMutableString *> *);

void test(Array *a, Array<NSString *> *b,
          Array<NSNumber *> *c) {
  a = b;
  c = a; // expected-warning  {{incompatible}}
  [a contains: [[NSNumber alloc] init]]; // expected-warning {{incompatible}}
  [a contains: [[NSString alloc] init]];
  doStuff(a); // expected-warning {{incompatible}}
}

void test2() {
  Array<NSString *> *a = getTypedStuff(); // expected-warning {{incompatible}}
}

void test3(Array *a, Array<NSString *> *b) {
  b = a;
  [a contains: [[NSNumber alloc] init]]; // expected-warning {{incompatible}}
  [a contains: [[NSString alloc] init]];
  doStuff(a); // expected-warning {{incompatible}}
}

void test4(id a, Array<NSString *> *b) {
  b = a;
  [a contains: [[NSNumber alloc] init]]; // expected-warning {{incompatible}}
  [a contains: [[NSString alloc] init]];
  doStuff(a); // expected-warning {{incompatible}}
}

void test5(id a, Array<NSString *> *b,
           Array<NSNumber *> *c) {
  a = b;
  c = a; // expected-warning {{incompatible}}
  [a contains: [[NSNumber alloc] init]]; // expected-warning {{incompatible}}
  [a contains: [[NSString alloc] init]];
  doStuff(a); // expected-warning {{incompatible}}
}

void test6(Array *m, Array<NSString *> *a,
           Array<NSMutableString *> *b) {
  if (getUnknown() == 5) {
    m = a;  
    [m contains: [[NSString alloc] init]];
  } else {
    m = b;
    [m contains: [[NSMutableString alloc] init]];
  }
  [m contains: [[NSString alloc] init]]; // expected-warning {{incompatible}}
  [m contains: [[NSMutableString alloc] init]];
}

void test7(Array *m, Array<NSString *> *a,
           Array<NSMutableString *> *b) {
  a = b;
  if (getUnknown() == 5) {
    m = a;  
    [m contains: [[NSString alloc] init]]; // expected-warning {{incompatible}}
  } else {
    m = b;
    [m contains: [[NSMutableString alloc] init]];
  }
}

void test8(id a, MutableArray<NSString *> *b) {
  b = a;
  doStuff(a); // expected-warning {{incompatible}}
}

void test9(Array<NSString *> *a,
           Array<NSMutableString *> *b) {
  b = (Array<NSMutableString *> *)a;
  [a contains: [[NSString alloc] init]]; // expected-warning {{incompatible}}
}

void test10(id d, MyMutableStringArray *a,
            MutableArray<NSString *> *b,
            MutableArray<NSNumber *> *c) {
  d = a;
  b = d;
  c = d; // expected-warning {{incompatible}}
}

void test11(id d, ExceptionalArray<NSString *> *a,
            MutableArray<NSString *> *b,
            MutableArray<NSNumber *> *c) {
  d = a;
  [d contains: [[NSString alloc] init]];
  [d contains: [[NSNumber alloc] init]]; // expected-warning {{incompatible}}
  b = d;
  c = d; // expected-warning {{incompatible}}
}

void test12(id d, ExceptionalArray<NSString *> *a,
            MutableArray<NSString *> *b,
            MutableArray<NSNumber *> *c) {
  a = d;
  [d contains: [[NSString alloc] init]];
  [d contains: [[NSNumber alloc] init]]; // expected-warning {{incompatible}}
  b = d;
  c = d; // expected-warning {{incompatible}}
}

void test13(id a) {
  withString(a);
  withMutableString(a);
  [a contains: [[NSString alloc] init]]; // expected-warning {{incompatible}}
}

void test14(id a) {
  withMutableString(a);
  withString(a);
  [a contains: [[NSString alloc] init]]; // expected-warning {{incompatible}}
}

void test15(LegacyArray *a) {
  withMutableString(a);
  withString(a);
  [a contains: [[NSString alloc] init]]; // expected-warning {{incompatible}}
}

void test16(LegacyMutableArray *a) {
  withString(a);
  withMutableString(a);
  [a contains: [[NSString alloc] init]]; // expected-warning {{incompatible}}
}

void test17(BuggyArray<NSMutableString *> *a) {
  withString(a);
  withMutableString(a);
  [a contains: [[NSString alloc] init]]; // expected-warning {{incompatible}}
}

void test18(BuggyMutableArray<NSMutableString *> *a) {
  withMutableString(a);
  withString(a);
  [a contains: [[NSString alloc] init]]; // expected-warning {{incompatible}}
}
