// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -emit-llvm -o %t %s
// RUN: grep '@"OBJC_LABEL_NONLAZY_CLASS_$" = private global \[1 x .*\] .*@"OBJC_CLASS_$_A".*, section "__DATA, __objc_nlclslist, regular, no_dead_strip", align 8' %t
// RUN: grep '@"OBJC_LABEL_NONLAZY_CATEGORY_$" = private global \[1 x .*\] .*@".01l_OBJC_$_CATEGORY_A_$_Cat".*, section "__DATA, __objc_nlcatlist, regular, no_dead_strip", align 8' %t
// rdar://16076729

@partial_interface A;
@partial_interface B;
@partial_interface C : A;

__attribute((objc_complete_definition))
@interface A @end
@implementation A
+(void) load {
}
@end

@interface A (Cat) @end
@implementation A (Cat)
+(void) load {
}
@end

__attribute((objc_complete_definition))
@interface B @end
@implementation B
-(void) load {
}
@end

@interface B (Cat) @end
@implementation B (Cat)
-(void) load {
}
@end

__attribute((objc_complete_definition))
@interface C : A @end
@implementation C
@end
