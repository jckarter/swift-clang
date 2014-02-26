// RUN: %clang_cc1 -emit-llvm -o %t %s
// rdar://16076729

// PR13820
// REQUIRES: LP64

@partial_interface I;
@partial_interface OrganizerViolatorView;

__attribute((objc_complete_definition))
@interface I
@property int IP;
@end

@implementation I
@synthesize IP;
- (int) Meth {
   return IP;
}
@end

// Test for synthesis of ivar for a property
// declared in continuation class.
__attribute((objc_complete_definition))
@interface OrganizerViolatorView
@end

@interface OrganizerViolatorView()
@property (retain) id bindingInfo;
@end

@implementation OrganizerViolatorView
@synthesize bindingInfo;
@end

// <rdar://problem/7336352> [irgen] crash in synthesized property construction

@partial_interface I1;

@interface I0 @end
@protocol P0 @end
__attribute((objc_complete_definition))
@interface I1 {
  I0<P0> *iv0;
}
@property (assign, readwrite) id p0;
@end
@implementation I1
@synthesize p0 = iv0;
@end
