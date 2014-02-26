// REQUIRES: x86-registered-target
// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -S -o %t %s 
// RUN: FileCheck < %t %s

// rdar://16076729

/** The problem looks like clang getting confused when a single translation unit 
    contains a protocol with a property and two classes that implement that protocol 
    and synthesize the property.
*/

@protocol Proto
@property (assign) id prop;
@end


@interface NSObject @end

@partial_interface Foo : NSObject;

__attribute((objc_complete_definition))
@interface Foo : NSObject <Proto> { int x; } @end

@partial_interface Bar : NSObject;

__attribute((objc_complete_definition))
@interface Bar : NSObject <Proto> @end

@implementation Foo
@synthesize prop;
@end

@implementation Bar
@synthesize prop;
@end

// CHECK: l_OBJC_$_INSTANCE_METHODS_Bar:
// CHECK-NEXT:        .long   24
// CHECK-NEXT:        .long   2
// CHECK-NEXT:        .quad   L_OBJC_METH_VAR_NAME_
// CHECK-NEXT:        .quad   L_OBJC_METH_VAR_TYPE_
// CHECK-NEXT:        .quad   "-[Bar prop]"
