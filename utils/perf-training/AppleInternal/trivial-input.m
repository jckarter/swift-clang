// RUN: %clang %xcode_objc_warnings -c %s -O0 -g -include %test_root/Cocoa_Prefix_Precompiled.h 
// Assumes prefix file is used.

@interface A : NSString
-(void) f0;
@end

@implementation A
-(void) f0 {}
@end
