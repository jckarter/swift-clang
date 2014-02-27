// RUN: %clang_cc1 -fsyntax-only -verify %s
typedef signed char BOOL;
typedef unsigned int NSUInteger;
typedef struct _NSZone NSZone;

@partial_interface NSObject;
@partial_interface NSTask:NSObject;
@partial_interface NSCell:NSObject;
@partial_interface NSActionCell:NSCell;
@partial_interface XCElementRootFace:NSObject;
@partial_interface XCElementFace:XCElementRootFace;
@partial_interface XCRASlice:XCElementFace;
@partial_interface XCElementDisplay:NSObject;
@partial_interface XCElementDisplayRect:XCElementDisplay;
@partial_interface XCElementDisplayFillerImage:XCElementDisplay;

@class NSInvocation, NSMethodSignature, NSCoder, NSString, NSEnumerator;

@protocol NSObject - (BOOL) isEqual:(id) object; - (id)init; @end
@protocol NSCopying - (id) copyWithZone:(NSZone *) zone; @end
@protocol NSCoding - (void) encodeWithCoder:(NSCoder *) aCoder; @end

__attribute((objc_complete_definition))
@interface NSObject < NSObject > {}
+(id) alloc;
@end

typedef float CGFloat;

__attribute((objc_complete_definition))
@interface NSTask:NSObject
- (id) init;
@end

typedef NSUInteger NSControlSize;
typedef struct __CFlags {} _CFlags;

__attribute((objc_complete_definition))
@interface NSCell:NSObject < NSCopying, NSCoding > {}
@end

__attribute((objc_complete_definition))
@interface NSActionCell:NSCell {} @end

@class NSAttributedString, NSFont, NSImage, NSSound;

typedef struct _XCElementInset {} XCElementInset;

@protocol XCElementP < NSObject >
-(BOOL) vertical;
@end

@protocol XCElementDisplayDelegateP;
@protocol XCElementDisplayDelegateP < NSObject >
-(void) configureForControlSize:(NSControlSize)size font:(NSFont *)font addDefaultSpace:(XCElementInset) additionalSpace;
@end

@protocol XCElementSpacerP < XCElementP >
@end

typedef NSObject < XCElementSpacerP > XCElementSpacer;

@protocol XCElementTogglerP < XCElementP > -(void) setDisplayed:(BOOL) displayed;
@end

typedef NSObject < XCElementTogglerP > XCElementToggler; // expected-note {{previous definition is here}}

__attribute((objc_complete_definition))
@interface XCElementRootFace:NSObject {} @end

__attribute((objc_complete_definition))
@interface XCElementFace:XCElementRootFace {} @end

@class XCElementToggler;  // expected-warning {{redefinition of forward class 'XCElementToggler' of a typedef name of an object type is ignored}}

__attribute((objc_complete_definition))
@interface XCRASlice:XCElementFace {} @end

@class XCElementSpacings;

__attribute((objc_complete_definition))
@interface XCElementDisplay:NSObject < XCElementDisplayDelegateP > {} @end
__attribute((objc_complete_definition))
@interface XCElementDisplayRect:XCElementDisplay {} @end

typedef XCElementDisplayRect XCElementGraphicsRect;

__attribute((objc_complete_definition))
@interface XCElementDisplayFillerImage:XCElementDisplay {} @end

@implementation XCRASlice
- (void) addSliceWithLabel:(NSString *)label statusKey:(NSString *)statusKey disclosed:(BOOL)disclosed
{
  static XCElementGraphicsRect *_sGraphicsDelegate = ((void *) 0);
  if (!_sGraphicsDelegate) {
    _sGraphicsDelegate =[[XCElementGraphicsRect alloc] init]; 
  }
}
@end
