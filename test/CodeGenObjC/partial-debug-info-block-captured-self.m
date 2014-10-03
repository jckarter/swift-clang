// RUN: %clang_cc1 -fblocks -g -emit-llvm -triple x86_64-apple-darwin -o - %s | FileCheck %s
//
// Test that debug location is generated for a captured "self" inside
// a block.
//
// This test is split into two parts, this one for the frontend, and
// then llvm/test/DebugInfo/debug-info-block-captured-self.ll to
// ensure that DW_AT_location is generated for the captured self.
@partial_interface S;
@partial_interface Mode;
@partial_interface Context;
@partial_interface ViewController;
@partial_interface Main : ViewController;
@partial_interface T;

@class T;
__attribute((objc_complete_definition))
@interface S
@end
__attribute((objc_complete_definition))
@interface Mode
-(int) count;
@end
__attribute((objc_complete_definition))
@interface Context
@end
__attribute((objc_complete_definition))
@interface ViewController
@property (nonatomic, readwrite, strong) Context *context;
@end
typedef enum {
    Unknown = 0,
} State;
__attribute((objc_complete_definition))
@interface Main : ViewController
{
    T * t1;
    T * t2;
}
@property(readwrite, nonatomic) State state;
@end
@implementation Main
- (id) initWithContext:(Context *) context
{
    t1 = [self.context withBlock:^(id obj){
        id *mode1;
	t2 = [mode1 withBlock:^(id object){
	    Mode *mode2 = object;
	    if ([mode2 count] != 0) {
	      self.state = 0;
	    }
	  }];
      }];
}
@end
// The important part of this test is that there is a dbg.value
// intrinsic associated with the implicit .block_descriptor argument
// of the block. We also test that this value gets alloca'd, so the
// register llocator won't accidentally kill it.

// outer block:
// CHECK: define internal void {{.*}}_block_invoke{{.*}}

// inner block:
// CHECK: define internal void {{.*}}_block_invoke{{.*}}
// CHECK:        %[[MEM1:.*]] = alloca i8*, align 8
// CHECK-NEXT:   %[[MEM2:.*]] = alloca i8*, align 8
// CHECK:        store i8* [[BLOCK_DESC:%.*]], i8** %[[MEM1]], align 8
// CHECK:        %[[TMP0:.*]] = load i8** %[[MEM1]]
// CHECK:        call void @llvm.dbg.value(metadata !{i8* %[[TMP0]]}, i64 0, metadata ![[BDMD:[0-9]+]], metadata !{{[0-9]+}})
// CHECK:        call void @llvm.dbg.declare(metadata !{i8* [[BLOCK_DESC]]}, metadata ![[BDMD:[0-9]+]], metadata !{{[0-9]+}})
// CHECK:        %[[TMP1:.*]] = bitcast
// CHECK-NEXT:   store
// CHECK:        call void @llvm.dbg.declare(metadata !{<{ i8*, i32, i32, i8*, %struct.__block_descriptor*, %0* }>** {{.*}}}, metadata ![[SELF:.*]], metadata !{{[0-9]+}})
// make sure we are still in the same function
// CHECK: define {{.*}}__copy_helper_block_
// Metadata
// CHECK:        ![[MAIN:.*]] = {{.*}}[ DW_TAG_structure_type ] [Main] [line 35,
// CHECK:        ![[PMAIN:.*]] = {{.*}}![[MAIN]]} ; [ DW_TAG_pointer_type ]{{.*}}from Main
// CHECK:        ![[BDMD]] = metadata {{.*}}.block_descriptor
// CHECK:        ![[SELF]] = {{.*}}![[PMAIN]]{{.*}}[ DW_TAG_auto_variable ] [self] [line 52]
