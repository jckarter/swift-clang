// RUN: %clang_cc1  -fsyntax-only -verify %s
// rdar://9181463

typedef struct objc_class *Class;

typedef struct objc_object {
    Class isa;
} *id;

@interface NSObject
+ (id) alloc;
@end


void foo(Class self) {
  [self alloc];
}

void bar(Class self) {
  Class y = self;
  [y alloc];
}

