// RUN: %clang_cc1 -x objective-c -triple x86_64-apple-darwin10 -fblocks -emit-llvm %s -o /dev/null
// RUN: %clang_cc1 -x objective-c++ -triple x86_64-apple-darwin10 -fblocks -emit-llvm %s -o /dev/null
// rdar://10111397

#if __has_feature(objc_bool)
#define YES __objc_yes
#define NO __objc_no
#else
#define YES             ((BOOL)1)
#define NO              ((BOOL)0)
#endif

#if __LP64__ || (TARGET_OS_EMBEDDED && !TARGET_OS_IPHONE) || TARGET_OS_WIN32 || NS_BUILD_32_LIKE_64
typedef unsigned long NSUInteger;
typedef long NSInteger;
#else
typedef unsigned int NSUInteger;
typedef int NSInteger;
#endif
typedef signed char BOOL;

@interface NSNumber @end

@interface NSNumber (NSNumberCreation)
#if __has_feature(objc_array_literals)
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
#endif
@end

@interface NSDate
+ (NSDate *) date;
@end

#if __has_feature(objc_dictionary_literals)
@interface NSDictionary
+ (id)dictionaryWithObjects:(const id [])objects forKeys:(const id [])keys count:(NSUInteger)cnt;
@end
#endif

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
  NSNumber *yesNumber1 = @YES;
  NSNumber *noNumber1 = @NO;
NSDictionary *dictionary = @{@"name" : NSUserName(), 
                             @"date" : [NSDate date] }; 
  return __objc_yes == __objc_no;
}

// rdar://10579122
typedef BOOL (^foo)(void);
extern void bar(foo a);

void baz(void) {
  bar(^(void) { return YES; });
}
