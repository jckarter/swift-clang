// RUN: %clang_cc1 -emit-llvm -o %t %s
// rdar://16076729
@partial_interface BASE;
@partial_interface PVR   : BASE;
@partial_interface PVRHandldler;
@partial_interface A;

__attribute((objc_complete_definition))
@interface BASE  {
@private
    void* _reserved;
}
@end

@class PVR;

__attribute((objc_complete_definition))
@interface PVRHandldler 
{
          PVR *_imageBrowser;
}
@end

@implementation PVRHandldler @end


__attribute((objc_complete_definition))
@interface PVR   : BASE
@end

@implementation PVR
@end

// Reopen of an interface after use.

__attribute((objc_complete_definition))
@interface A { 
@public 
  int x; 
} 
@property int p0;
@end

int f0(A *a) { 
  return a.p0; 
}

@implementation A
@synthesize p0 = _p0;
@end

__attribute((objc_complete_definition))
@interface B
@end
@class B;
@implementation B
@end
