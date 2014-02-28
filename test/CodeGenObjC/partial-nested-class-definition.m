// RUN: %clang_cc1 -emit-llvm -o - %s | FileCheck %s
// rdar://16076729

@partial_interface Root;

@interface Sub1  : Root @end

@partial_interface Sub2 : Sub1;

@interface Sub3 : Sub2 @end

@partial_interface Sub4 : Sub3;

@interface Sub5 : Sub4;

@property int P1;

- (id) Meth;

@property (copy) id P2;

@end

__attribute((objc_complete_definition))
@interface Root @end

__attribute((objc_complete_definition))
@interface Sub2 : Sub1 @end

__attribute((objc_complete_definition))
@interface Sub4  : Sub3 @end

@implementation Sub5
- (id) Meth { return 0; }
@end
// CHECK: define internal i8* @"\01-[Sub5 Meth]"
// CHECK: define internal i32 @"\01-[Sub5 P1]"
// CHECK: define internal void @"\01-[Sub5 setP1:]"
// CHECK: define internal i8* @"\01-[Sub5 P2]"
// CHECK: define internal void @"\01-[Sub5 setP2:]"
