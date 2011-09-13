// RUN: %clang_cc1 -x objective-c -emit-llvm %s -o /dev/null
// RUN: %clang_cc1 -x objective-c++ -emit-llvm %s -o /dev/null
// rdar://10111397

typedef unsigned long NSUInteger;
typedef long NSInteger;
typedef unsigned char BOOL;

@interface NSNumber @end

@interface NSNumber (NSNumberCreation)
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
+ (NSNumber *)numberWithInteger:(NSInteger)value ;
+ (NSNumber *)numberWithUnsignedInteger:(NSUInteger)value ;
@end

int main() {
  NSNumber *aNumber = @'a';
  NSNumber *fortyTwo = @42;
  NSNumber *fortyTwoUnsigned = @42u;
  NSNumber *fortyTwoLong = @42l;
  NSNumber *fortyTwoLongLong = @42ll;
  NSNumber *piFloat = @3.141592654f;
  NSNumber *piDouble = @3.1415926535;
  NSNumber *yesNumber = @__yes;
  NSNumber *noNumber = @__no;
#ifdef __cplusplus
  NSNumber *trueNumber = @__yes;
  NSNumber *falseNumber = @__no;
#endif
}
