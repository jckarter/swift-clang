// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -fobjc-nonfragile-abi -fsyntax-only -fobjc-arc -fobjc-runtime-has-weak -x objective-c %s.result
// RUN: arcmt-test --args -triple x86_64-apple-macosx10.7 -fobjc-nonfragile-abi -fsyntax-only %s > %t
// RUN: diff %t %s.result

#include "Common.h"

__attribute__((objc_arc_weak_reference_unavailable))
@interface WeakOptOut
@end

@class _NSCachedAttributedString;
typedef _NSCachedAttributedString *BadClassForWeak;

@class Forw;

@interface Foo : NSObject {
  Foo *x, *w, *q1, *q2;
  Foo *z1, *__unsafe_unretained z2;
  WeakOptOut *oo;
  BadClassForWeak bcw;
  id not_safe1;
  NSObject *not_safe2;
  Forw *not_safe3;
}
@property (readonly,assign) Foo *x;
@property (assign) Foo *w;
@property (assign) Foo *q1, *q2;
@property (assign) Foo *z1, *z2;
@property (assign) WeakOptOut *oo;
@property (assign) BadClassForWeak bcw;
@property (assign) id not_safe1;
@property (assign) NSObject *not_safe2;
@property (assign) Forw *not_safe3;
@end

@implementation Foo
@synthesize x,w,q1,q2,z1,z2,oo,bcw,not_safe1,not_safe2,not_safe3;
@end
