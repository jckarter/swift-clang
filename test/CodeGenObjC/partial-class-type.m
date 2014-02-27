// RUN: %clang_cc1 -triple x86_64-unknown-unknown -fobjc-runtime=macosx-fragile-10.5 -emit-llvm -o - %s
// RUN: %clang_cc1 -triple i386-apple-darwin9 -fobjc-runtime=macosx-fragile-10.5 -emit-llvm -o - %s
// RUN: %clang_cc1 -triple x86_64-apple-darwin9 -fobjc-runtime=macosx-fragile-10.5 -emit-llvm -o - %s

__attribute((objc_complete_definition))
@interface I0 {
  struct { int a; } a;
}
@end 

@partial_interface I0;

@class I2;

@partial_interface I2 : I0;

@interface I1 {
  I2 *_imageBrowser;
}
@end 

@implementation I1 
@end 

__attribute((objc_complete_definition))
@interface I2 : I0 
@end 

@implementation I2 
@end 


// Implementations without interface declarations.
// rdar://6804402
@partial_interface foo;
@class foo;
__attribute((objc_complete_definition))
@interface foo @end
@implementation foo 
@end

@implementation bar
@end

