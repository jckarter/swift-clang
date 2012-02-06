// RUN: %clang_cc1 -triple x86_64-apple-darwin10 %s -fsyntax-only -Wobjc-cocoa-api -verify
// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -fobjc-arc %s -fsyntax-only -Wobjc-cocoa-api -verify
// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -x objective-c++ %s.fixed -fsyntax-only
// RUN: cp %s %t.mm
// RUN: %clang_cc1 -triple x86_64-apple-darwin10 %t.mm -fixit -Wobjc-cocoa-api
// RUN: diff %s.fixed %t.mm
// RUN: cp %s %t.mm
// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -fobjc-arc %t.mm -fixit -Wobjc-cocoa-api
// RUN: diff %s.fixed %t.mm

#define YES __objc_yes
#define NO __objc_no

typedef long NSInteger;
typedef unsigned long NSUInteger;
typedef signed char BOOL;
#define nil ((void*) 0)

@interface NSObject
+ (id)alloc;
@end

@interface NSNumber : NSObject
@end

@interface NSNumber (NSNumberCreation)
- (id)initWithChar:(char)value;
- (id)initWithUnsignedChar:(unsigned char)value;
- (id)initWithShort:(short)value;
- (id)initWithUnsignedShort:(unsigned short)value;
- (id)initWithInt:(int)value;
- (id)initWithUnsignedInt:(unsigned int)value;
- (id)initWithLong:(long)value;
- (id)initWithUnsignedLong:(unsigned long)value;
- (id)initWithLongLong:(long long)value;
- (id)initWithUnsignedLongLong:(unsigned long long)value;
- (id)initWithFloat:(float)value;
- (id)initWithDouble:(double)value;
- (id)initWithBool:(BOOL)value;
- (id)initWithInteger:(NSInteger)value;
- (id)initWithUnsignedInteger:(NSUInteger)value;

+ (NSNumber *)numberWithChar:(char)value;
+ (NSNumber *)numberWithUnsignedChar:(unsigned char)value;
+ (NSNumber *)numberWithShort:(short)value;
+ (NSNumber *)numberWithUnsignedShort:(unsigned short)value;
+ (NSNumber *)numberWithInt:(int)value;
+ (NSNumber *)numberWithUnsignedInt:(unsigned int)value;
+ (NSNumber *)numberWithLong:(long)value;
+ (NSNumber *)numberWithUnsignedLong:(unsigned long)value;
+ (NSNumber *)numberWithLongLong:(long long)value;
+ (NSNumber *)numberWithUnsignedLongLong:(unsigned long long)value;
+ (NSNumber *)numberWithFloat:(float)value;
+ (NSNumber *)numberWithDouble:(double)value;
+ (NSNumber *)numberWithBool:(BOOL)value;
+ (NSNumber *)numberWithInteger:(NSInteger)value;
+ (NSNumber *)numberWithUnsignedInteger:(NSUInteger)value;
@end

#define VAL_INT 2
#define VAL_UINT 2U
#define VAL_CHAR 'a'

void foo() {
  [NSNumber numberWithChar:'a']; // expected-warning {{legacy}}
  [NSNumber numberWithChar:L'a'];
  [NSNumber numberWithChar:2];
  [NSNumber numberWithChar:2U];
  [NSNumber numberWithChar:2u];
  [NSNumber numberWithChar:2L];
  [NSNumber numberWithChar:2l];
  [NSNumber numberWithChar:2LL];
  [NSNumber numberWithChar:2ll];
  [NSNumber numberWithChar:2ul];
  [NSNumber numberWithChar:2lu];
  [NSNumber numberWithChar:2ull];
  [NSNumber numberWithChar:2llu];
  [NSNumber numberWithChar:2.0];
  [NSNumber numberWithChar:2.0f];
  [NSNumber numberWithChar:2.0F];
  [NSNumber numberWithChar:2.0l];
  [NSNumber numberWithChar:2.0L];
  [NSNumber numberWithChar:0x2f];
  [NSNumber numberWithChar:04];
  [NSNumber numberWithChar:0];
  [NSNumber numberWithChar:0.0];
  [NSNumber numberWithChar:YES];
  [NSNumber numberWithChar:NO];
  [NSNumber numberWithChar:true];
  [NSNumber numberWithChar:false];
  [NSNumber numberWithChar:VAL_INT];
  [NSNumber numberWithChar:VAL_UINT];
  [NSNumber numberWithChar:VAL_CHAR]; // expected-warning {{legacy}}

  [NSNumber numberWithUnsignedChar:'a'];
  [NSNumber numberWithUnsignedChar:L'a'];
  [NSNumber numberWithUnsignedChar:2];
  [NSNumber numberWithUnsignedChar:2U];
  [NSNumber numberWithUnsignedChar:2u];
  [NSNumber numberWithUnsignedChar:2L];
  [NSNumber numberWithUnsignedChar:2l];
  [NSNumber numberWithUnsignedChar:2LL];
  [NSNumber numberWithUnsignedChar:2ll];
  [NSNumber numberWithUnsignedChar:2ul];
  [NSNumber numberWithUnsignedChar:2lu];
  [NSNumber numberWithUnsignedChar:2ull];
  [NSNumber numberWithUnsignedChar:2llu];
  [NSNumber numberWithUnsignedChar:2.0];
  [NSNumber numberWithUnsignedChar:2.0f];
  [NSNumber numberWithUnsignedChar:2.0F];
  [NSNumber numberWithUnsignedChar:2.0l];
  [NSNumber numberWithUnsignedChar:2.0L];
  [NSNumber numberWithUnsignedChar:0x2f];
  [NSNumber numberWithUnsignedChar:04];
  [NSNumber numberWithUnsignedChar:0];
  [NSNumber numberWithUnsignedChar:0.0];
  [NSNumber numberWithUnsignedChar:YES];
  [NSNumber numberWithUnsignedChar:NO];
  [NSNumber numberWithUnsignedChar:true];
  [NSNumber numberWithUnsignedChar:false];
  [NSNumber numberWithUnsignedChar:VAL_INT];
  [NSNumber numberWithUnsignedChar:VAL_UINT];
  [NSNumber numberWithUnsignedChar:VAL_CHAR];

  [NSNumber numberWithShort:'a'];
  [NSNumber numberWithShort:L'a'];
  [NSNumber numberWithShort:2];
  [NSNumber numberWithShort:2U];
  [NSNumber numberWithShort:2u];
  [NSNumber numberWithShort:2L];
  [NSNumber numberWithShort:2l];
  [NSNumber numberWithShort:2LL];
  [NSNumber numberWithShort:2ll];
  [NSNumber numberWithShort:2ul];
  [NSNumber numberWithShort:2lu];
  [NSNumber numberWithShort:2ull];
  [NSNumber numberWithShort:2llu];
  [NSNumber numberWithShort:2.0];
  [NSNumber numberWithShort:2.0f];
  [NSNumber numberWithShort:2.0F];
  [NSNumber numberWithShort:2.0l];
  [NSNumber numberWithShort:2.0L];
  [NSNumber numberWithShort:0x2f];
  [NSNumber numberWithShort:04];
  [NSNumber numberWithShort:0];
  [NSNumber numberWithShort:0.0];
  [NSNumber numberWithShort:YES];
  [NSNumber numberWithShort:NO];
  [NSNumber numberWithShort:true];
  [NSNumber numberWithShort:false];
  [NSNumber numberWithShort:VAL_INT];
  [NSNumber numberWithShort:VAL_UINT];

  [NSNumber numberWithUnsignedShort:'a'];
  [NSNumber numberWithUnsignedShort:L'a'];
  [NSNumber numberWithUnsignedShort:2];
  [NSNumber numberWithUnsignedShort:2U];
  [NSNumber numberWithUnsignedShort:2u];
  [NSNumber numberWithUnsignedShort:2L];
  [NSNumber numberWithUnsignedShort:2l];
  [NSNumber numberWithUnsignedShort:2LL];
  [NSNumber numberWithUnsignedShort:2ll];
  [NSNumber numberWithUnsignedShort:2ul];
  [NSNumber numberWithUnsignedShort:2lu];
  [NSNumber numberWithUnsignedShort:2ull];
  [NSNumber numberWithUnsignedShort:2llu];
  [NSNumber numberWithUnsignedShort:2.0];
  [NSNumber numberWithUnsignedShort:2.0f];
  [NSNumber numberWithUnsignedShort:2.0F];
  [NSNumber numberWithUnsignedShort:2.0l];
  [NSNumber numberWithUnsignedShort:2.0L];
  [NSNumber numberWithUnsignedShort:0x2f];
  [NSNumber numberWithUnsignedShort:04];
  [NSNumber numberWithUnsignedShort:0];
  [NSNumber numberWithUnsignedShort:0.0];
  [NSNumber numberWithUnsignedShort:YES];
  [NSNumber numberWithUnsignedShort:NO];
  [NSNumber numberWithUnsignedShort:true];
  [NSNumber numberWithUnsignedShort:false];
  [NSNumber numberWithUnsignedShort:VAL_INT];
  [NSNumber numberWithUnsignedShort:VAL_UINT];

  [NSNumber numberWithInt:'a'];
  [NSNumber numberWithInt:L'a'];
  [NSNumber numberWithInt:2]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:2U]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:2u]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:2L]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:2l]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:2LL]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:2ll]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:2ul]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:2lu]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:2ull]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:2llu]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:2.0];
  [NSNumber numberWithInt:2.0f];
  [NSNumber numberWithInt:2.0F];
  [NSNumber numberWithInt:2.0l];
  [NSNumber numberWithInt:2.0L];
  [NSNumber numberWithInt:0x2f]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:04]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:0]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:0.0];
  [NSNumber numberWithInt:YES];
  [NSNumber numberWithInt:NO];
  [NSNumber numberWithInt:true];
  [NSNumber numberWithInt:false];
  [NSNumber numberWithInt:VAL_INT]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:VAL_UINT];

  (void)[[NSNumber alloc] initWithInt:2]; // expected-warning {{legacy}}
  (void)[[NSNumber alloc] initWithInt:2U]; // expected-warning {{legacy}}

  [NSNumber numberWithInt:+2]; // expected-warning {{legacy}}
  [NSNumber numberWithInt:-2]; // expected-warning {{legacy}}

  [NSNumber numberWithUnsignedInt:'a'];
  [NSNumber numberWithUnsignedInt:L'a'];
  [NSNumber numberWithUnsignedInt:2]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedInt:2U]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedInt:2u]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedInt:2L]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedInt:2l]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedInt:2LL]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedInt:2ll]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedInt:2ul]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedInt:2lu]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedInt:2ull]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedInt:2llu]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedInt:2.0];
  [NSNumber numberWithUnsignedInt:2.0f];
  [NSNumber numberWithUnsignedInt:2.0F];
  [NSNumber numberWithUnsignedInt:2.0l];
  [NSNumber numberWithUnsignedInt:2.0L];
  [NSNumber numberWithUnsignedInt:0x2f]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedInt:04]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedInt:0]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedInt:0.0];
  [NSNumber numberWithUnsignedInt:YES];
  [NSNumber numberWithUnsignedInt:NO];
  [NSNumber numberWithUnsignedInt:true];
  [NSNumber numberWithUnsignedInt:false];
  [NSNumber numberWithUnsignedInt:VAL_INT];
  [NSNumber numberWithUnsignedInt:VAL_UINT]; // expected-warning {{legacy}}

  [NSNumber numberWithLong:'a'];
  [NSNumber numberWithLong:L'a'];
  [NSNumber numberWithLong:2]; // expected-warning {{legacy}}
  [NSNumber numberWithLong:2U]; // expected-warning {{legacy}}
  [NSNumber numberWithLong:2u]; // expected-warning {{legacy}}
  [NSNumber numberWithLong:2L]; // expected-warning {{legacy}}
  [NSNumber numberWithLong:2l]; // expected-warning {{legacy}}
  [NSNumber numberWithLong:2LL]; // expected-warning {{legacy}}
  [NSNumber numberWithLong:2ll]; // expected-warning {{legacy}}
  [NSNumber numberWithLong:2ul]; // expected-warning {{legacy}}
  [NSNumber numberWithLong:2lu]; // expected-warning {{legacy}}
  [NSNumber numberWithLong:2ull]; // expected-warning {{legacy}}
  [NSNumber numberWithLong:2llu]; // expected-warning {{legacy}}
  [NSNumber numberWithLong:2.0];
  [NSNumber numberWithLong:2.0f];
  [NSNumber numberWithLong:2.0F];
  [NSNumber numberWithLong:2.0l];
  [NSNumber numberWithLong:2.0L];
  [NSNumber numberWithLong:0x2f]; // expected-warning {{legacy}}
  [NSNumber numberWithLong:04]; // expected-warning {{legacy}}
  [NSNumber numberWithLong:0]; // expected-warning {{legacy}}
  [NSNumber numberWithLong:0.0];
  [NSNumber numberWithLong:YES];
  [NSNumber numberWithLong:NO];
  [NSNumber numberWithLong:true];
  [NSNumber numberWithLong:false];
  [NSNumber numberWithLong:VAL_INT];
  [NSNumber numberWithLong:VAL_UINT];

  [NSNumber numberWithUnsignedLong:'a'];
  [NSNumber numberWithUnsignedLong:L'a'];
  [NSNumber numberWithUnsignedLong:2]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLong:2U]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLong:2u]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLong:2L]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLong:2l]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLong:2LL]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLong:2ll]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLong:2ul]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLong:2lu]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLong:2ull]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLong:2llu]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLong:2.0];
  [NSNumber numberWithUnsignedLong:2.0f];
  [NSNumber numberWithUnsignedLong:2.0F];
  [NSNumber numberWithUnsignedLong:2.0l];
  [NSNumber numberWithUnsignedLong:2.0L];
  [NSNumber numberWithUnsignedLong:0x2f]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLong:04]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLong:0]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLong:0.0];
  [NSNumber numberWithUnsignedLong:YES];
  [NSNumber numberWithUnsignedLong:NO];
  [NSNumber numberWithUnsignedLong:true];
  [NSNumber numberWithUnsignedLong:false];
  [NSNumber numberWithUnsignedLong:VAL_INT];
  [NSNumber numberWithUnsignedLong:VAL_UINT];

  [NSNumber numberWithLongLong:'a'];
  [NSNumber numberWithLongLong:L'a'];
  [NSNumber numberWithLongLong:2]; // expected-warning {{legacy}}
  [NSNumber numberWithLongLong:2U]; // expected-warning {{legacy}}
  [NSNumber numberWithLongLong:2u]; // expected-warning {{legacy}}
  [NSNumber numberWithLongLong:2L]; // expected-warning {{legacy}}
  [NSNumber numberWithLongLong:2l]; // expected-warning {{legacy}}
  [NSNumber numberWithLongLong:2LL]; // expected-warning {{legacy}}
  [NSNumber numberWithLongLong:2ll]; // expected-warning {{legacy}}
  [NSNumber numberWithLongLong:2ul]; // expected-warning {{legacy}}
  [NSNumber numberWithLongLong:2lu]; // expected-warning {{legacy}}
  [NSNumber numberWithLongLong:2ull]; // expected-warning {{legacy}}
  [NSNumber numberWithLongLong:2llu]; // expected-warning {{legacy}}
  [NSNumber numberWithLongLong:2.0];
  [NSNumber numberWithLongLong:2.0f];
  [NSNumber numberWithLongLong:2.0F];
  [NSNumber numberWithLongLong:2.0l];
  [NSNumber numberWithLongLong:2.0L];
  [NSNumber numberWithLongLong:0x2f]; // expected-warning {{legacy}}
  [NSNumber numberWithLongLong:04]; // expected-warning {{legacy}}
  [NSNumber numberWithLongLong:0]; // expected-warning {{legacy}}
  [NSNumber numberWithLongLong:0.0];
  [NSNumber numberWithLongLong:YES];
  [NSNumber numberWithLongLong:NO];
  [NSNumber numberWithLongLong:true];
  [NSNumber numberWithLongLong:false];
  [NSNumber numberWithLongLong:VAL_INT];
  [NSNumber numberWithLongLong:VAL_UINT];

  [NSNumber numberWithUnsignedLongLong:'a'];
  [NSNumber numberWithUnsignedLongLong:L'a'];
  [NSNumber numberWithUnsignedLongLong:2]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLongLong:2U]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLongLong:2u]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLongLong:2L]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLongLong:2l]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLongLong:2LL]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLongLong:2ll]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLongLong:2ul]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLongLong:2lu]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLongLong:2ull]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLongLong:2llu]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLongLong:2.0];
  [NSNumber numberWithUnsignedLongLong:2.0f];
  [NSNumber numberWithUnsignedLongLong:2.0F];
  [NSNumber numberWithUnsignedLongLong:2.0l];
  [NSNumber numberWithUnsignedLongLong:2.0L];
  [NSNumber numberWithUnsignedLongLong:0x2f]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLongLong:04]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLongLong:0]; // expected-warning {{legacy}}
  [NSNumber numberWithUnsignedLongLong:0.0];
  [NSNumber numberWithUnsignedLongLong:YES];
  [NSNumber numberWithUnsignedLongLong:NO];
  [NSNumber numberWithUnsignedLongLong:true];
  [NSNumber numberWithUnsignedLongLong:false];
  [NSNumber numberWithUnsignedLongLong:VAL_INT];
  [NSNumber numberWithUnsignedLongLong:VAL_UINT];

  [NSNumber numberWithFloat:'a'];
  [NSNumber numberWithFloat:L'a'];
  [NSNumber numberWithFloat:2]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:2U]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:2u]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:2L]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:2l]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:2LL]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:2ll]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:2ul]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:2lu]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:2ull]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:2llu]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:2.0]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:2.0f]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:2.0F]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:2.0l]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:2.0L]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:0x2f];
  [NSNumber numberWithFloat:04];
  [NSNumber numberWithFloat:0]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:0.0]; // expected-warning {{legacy}}
  [NSNumber numberWithFloat:YES];
  [NSNumber numberWithFloat:NO];
  [NSNumber numberWithFloat:true];
  [NSNumber numberWithFloat:false];
  [NSNumber numberWithFloat:VAL_INT];
  [NSNumber numberWithFloat:VAL_UINT];

  [NSNumber numberWithDouble:'a'];
  [NSNumber numberWithDouble:L'a'];
  [NSNumber numberWithDouble:2]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:2U]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:2u]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:2L]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:2l]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:2LL]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:2ll]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:2ul]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:2lu]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:2ull]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:2llu]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:2.0]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:2.0f]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:2.0F]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:2.0l]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:2.0L]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:0x2f];
  [NSNumber numberWithDouble:04];
  [NSNumber numberWithDouble:0]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:0.0]; // expected-warning {{legacy}}
  [NSNumber numberWithDouble:YES];
  [NSNumber numberWithDouble:NO];
  [NSNumber numberWithDouble:true];
  [NSNumber numberWithDouble:false];
  [NSNumber numberWithDouble:VAL_INT];
  [NSNumber numberWithDouble:VAL_UINT];

  [NSNumber numberWithBool:'a'];
  [NSNumber numberWithBool:L'a'];
  [NSNumber numberWithBool:2];
  [NSNumber numberWithBool:2U];
  [NSNumber numberWithBool:2u];
  [NSNumber numberWithBool:2L];
  [NSNumber numberWithBool:2l];
  [NSNumber numberWithBool:2LL];
  [NSNumber numberWithBool:2ll];
  [NSNumber numberWithBool:2ul];
  [NSNumber numberWithBool:2lu];
  [NSNumber numberWithBool:2ull];
  [NSNumber numberWithBool:2llu];
  [NSNumber numberWithBool:2.0];
  [NSNumber numberWithBool:2.0f];
  [NSNumber numberWithBool:2.0F];
  [NSNumber numberWithBool:2.0l];
  [NSNumber numberWithBool:2.0L];
  [NSNumber numberWithBool:0x2f];
  [NSNumber numberWithBool:04];
  [NSNumber numberWithBool:0];
  [NSNumber numberWithBool:0.0];
  [NSNumber numberWithBool:YES]; // expected-warning {{legacy}}
  [NSNumber numberWithBool:NO]; // expected-warning {{legacy}}
  [NSNumber numberWithBool:true]; // expected-warning {{legacy}}
  [NSNumber numberWithBool:false]; // expected-warning {{legacy}}
  [NSNumber numberWithBool:VAL_INT];
  [NSNumber numberWithBool:VAL_UINT];

  [NSNumber numberWithInteger:'a'];
  [NSNumber numberWithInteger:L'a'];
  [NSNumber numberWithInteger:2];
  [NSNumber numberWithInteger:2U];
  [NSNumber numberWithInteger:2u];
  [NSNumber numberWithInteger:2L];
  [NSNumber numberWithInteger:2l];
  [NSNumber numberWithInteger:2LL];
  [NSNumber numberWithInteger:2ll];
  [NSNumber numberWithInteger:2ul];
  [NSNumber numberWithInteger:2lu];
  [NSNumber numberWithInteger:2ull];
  [NSNumber numberWithInteger:2llu];
  [NSNumber numberWithInteger:2.0];
  [NSNumber numberWithInteger:2.0f];
  [NSNumber numberWithInteger:2.0F];
  [NSNumber numberWithInteger:2.0l];
  [NSNumber numberWithInteger:2.0L];
  [NSNumber numberWithInteger:0x2f];
  [NSNumber numberWithInteger:04];
  [NSNumber numberWithInteger:0];
  [NSNumber numberWithInteger:0.0];
  [NSNumber numberWithInteger:YES];
  [NSNumber numberWithInteger:NO];
  [NSNumber numberWithInteger:true];
  [NSNumber numberWithInteger:false];
  [NSNumber numberWithInteger:VAL_INT];
  [NSNumber numberWithInteger:VAL_UINT];

  [NSNumber numberWithUnsignedInteger:'a'];
  [NSNumber numberWithUnsignedInteger:L'a'];
  [NSNumber numberWithUnsignedInteger:2];
  [NSNumber numberWithUnsignedInteger:2U];
  [NSNumber numberWithUnsignedInteger:2u];
  [NSNumber numberWithUnsignedInteger:2L];
  [NSNumber numberWithUnsignedInteger:2l];
  [NSNumber numberWithUnsignedInteger:2LL];
  [NSNumber numberWithUnsignedInteger:2ll];
  [NSNumber numberWithUnsignedInteger:2ul];
  [NSNumber numberWithUnsignedInteger:2lu];
  [NSNumber numberWithUnsignedInteger:2ull];
  [NSNumber numberWithUnsignedInteger:2llu];
  [NSNumber numberWithUnsignedInteger:2.0];
  [NSNumber numberWithUnsignedInteger:2.0f];
  [NSNumber numberWithUnsignedInteger:2.0F];
  [NSNumber numberWithUnsignedInteger:2.0l];
  [NSNumber numberWithUnsignedInteger:2.0L];
  [NSNumber numberWithUnsignedInteger:0x2f];
  [NSNumber numberWithUnsignedInteger:04];
  [NSNumber numberWithUnsignedInteger:0];
  [NSNumber numberWithUnsignedInteger:0.0];
  [NSNumber numberWithUnsignedInteger:YES];
  [NSNumber numberWithUnsignedInteger:NO];
  [NSNumber numberWithUnsignedInteger:true];
  [NSNumber numberWithUnsignedInteger:false];
  [NSNumber numberWithUnsignedInteger:VAL_INT];
  [NSNumber numberWithUnsignedInteger:VAL_UINT];
}
