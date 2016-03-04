// RUN: rm -rf %t.idx
// RUN: %clang_cc1 %s -index-store-path %t.idx
// RUN: c-index-test core -print-record %t.idx | FileCheck %s

@class MyCls;

@interface MyCls
@end

// CHECK: [[@LINE+2]]:6 | function/C | c:@F@foo#*$objc(cs)MyCls# | Decl | rel: 0
// CHECK: [[@LINE+1]]:10 | objc-class/ObjC | c:objc(cs)MyCls | Ref | rel: 0
void foo(MyCls *p);
