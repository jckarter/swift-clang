// RUN: %clang_cc1 -fobjc-no-direct-access-isa -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple arm64-apple-darwin9 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple arm64-apple-ios -fsyntax-only -verify %s
// rdar://10709102

typedef struct objc_object {
  struct objc_class *isa;
} *id;

@interface NSObject {
  struct objc_class *isa;
}
@end
@interface Whatever : NSObject
+self;
@end

static void func() {
 
  id x;

  [(*x).isa self]; // expected-error {{direct access to objective-c's isa is not allowed - use object_setClass() and object_getClass() instead}}
  [x->isa self];   // expected-error {{direct access to objective-c's isa is not allowed - use object_setClass() and object_getClass() instead}}
}
