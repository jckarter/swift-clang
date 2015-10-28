#import <stdio.h>
#import <objc/NSObject.h>

void foo() {

  NSObject *x;
  [x 
}

// FIXME: it would be great to test more of Cocoa, but it's too slow.

// CHECK-TOP-LEVEL: macro definition:{TypedText EOF} (70)
// CHECK-TOP-LEVEL: TypedefDecl:{TypedText FILE} (50)
// CHECK-TOP-LEVEL: ObjCInterfaceDecl:{TypedText NSObject} (50)
// CHECK-STMT: macro definition:{TypedText EOF} (70)
// CHECK-STMT: TypedefDecl:{TypedText FILE} (50)
// CHECK-STMT: ObjCInterfaceDecl:{TypedText NSObject} (50)
// CHECK-METHOD: ObjCInstanceMethodDecl:{ResultType BOOL}{TypedText isEqual:}{Placeholder (id)} (37)

// RUN: rm -rf %t
// RUN: CINDEXTEST_EDITING=1 c-index-test -code-completion-at=%s:3:1 %s -fmodules -fmodules-cache-path=%t \
// RUN:     | FileCheck %s -check-prefix=CHECK-TOP-LEVEL

// RUN: CINDEXTEST_EDITING=1 c-index-test -code-completion-at=%s:5:1 %s -fmodules -fmodules-cache-path=%t \
// RUN:     | FileCheck %s -check-prefix=CHECK-STMT

// RUN: CINDEXTEST_EDITING=1 c-index-test -code-completion-at=%s:7:6 %s -fmodules -fmodules-cache-path=%t \
// RUN:     | FileCheck %s -check-prefix=CHECK-METHOD
