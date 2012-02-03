// RUN: %clang_cc1  -fsyntax-only -fblocks -verify %s
// rdar://10111397

#if __LP64__ || (TARGET_OS_EMBEDDED && !TARGET_OS_IPHONE) || TARGET_OS_WIN32 || NS_BUILD_32_LIKE_64
typedef unsigned long NSUInteger;
#else
typedef unsigned int NSUInteger;
#endif

@interface NSNumber
+ (NSNumber *)numberWithChar:(char)value;
+ (NSNumber *)numberWithInt:(int)value;
+ (NSNumber *)numberWithFloat:(float)value;
@end

int main() {
  NSNumber * N = @3.1415926535;  // expected-error {{declaration of 'numberWithDouble:' is missing in NSNumber class}}
  NSNumber *noNumber = @__objc_yes; // expected-error {{declaration of 'numberWithBool:' is missing in NSNumber class}}
  NSNumber * NInt = @1000;
  NSNumber * NLongDouble = @1000.0l; // expected-error{{'long double' is not a valid literal type for NSNumber}}
  id character = @ 'a';

  NSNumber *NNegativeInt = @-1000;
  NSNumber *NPositiveInt = @+1000;
  NSNumber *NMaskInt = @~1000;
  NSNumber *NNegativeFloat = @-1000.1f;
  NSNumber *NPositiveFloat = @+1000.1f;
  NSNumber *NMaskFloat = @~1000.1f; // expected-error{{invalid argument type 'float' to unary expression}}

  int five = 5;
  @-five; // expected-error{{@- must be followed by a number to form an NSNumber object}}
  @+five; // expected-error{{@+ must be followed by a number to form an NSNumber object}}
  @~five; // expected-error{{@~ must be followed by a number to form an NSNumber object}}
}

// Dictionary test
@class NSDictionary;

NSDictionary *err() {
  return @{@"name" : @"value"}; // expected-error {{declaration of 'dictionaryWithObjects:forKeys:count:' is missing in NSDictionary class}}
}

@interface NSDate
+ (NSDate *) date;
@end

@interface NSDictionary
+ (id)dictionaryWithObjects:(const id [])objects forKeys:(const id [])keys count:(NSUInteger)cnt;
@end

id NSUserName();

int Int();

NSDictionary * blocks() {
  return @{ @"task" : ^ { return 17; } };
}

NSDictionary * warn() {
  NSDictionary *dictionary = @{@"name" : NSUserName(),
                               @"date" : [NSDate date] };
  return @{@"name" : Int()}; // expected-error {{collection element of type 'int' is not an Objective-C object}}
}
