// RUN: %clang_cc1 -x objective-c -emit-llvm %s -o /dev/null
// RUN: %clang_cc1 -x objective-c++ -emit-llvm %s -o /dev/null
// rdar://10111397

#if __LP64__ || (TARGET_OS_EMBEDDED && !TARGET_OS_IPHONE) || TARGET_OS_WIN32 || NS_BUILD_32_LIKE_64
typedef unsigned long NSUInteger;
typedef long NSInteger;
#else
typedef unsigned int NSUInteger;
typedef int NSInteger;
#endif
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

@interface NSDate
+ (NSDate *) date;
@end

@interface NSDictionary
+ (id)dictionaryWithObjects:(const id [])objects forKeys:(const id [])keys count:(NSUInteger)cnt;
@end

id NSUserName();

int main() {
  NSNumber *aNumber = @'a';
  NSNumber *fortyTwo = @42;
  NSNumber *fortyTwoUnsigned = @42u;
  NSNumber *fortyTwoLong = @42l;
  NSNumber *fortyTwoLongLong = @42ll;
  NSNumber *piFloat = @3.141592654f;
  NSNumber *piDouble = @3.1415926535;
  NSNumber *yesNumber = @__objc_yes;
  NSNumber *noNumber = @__objc_no;
NSDictionary *dictionary = @{@"name" : NSUserName(), 
                             @"date" : [NSDate date] }; 
  return __objc_yes == __objc_no;
}
