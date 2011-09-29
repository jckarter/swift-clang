// RUN: %clang_cc1 -fsyntax-only -verify %s

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
@end

@interface NSArray
@end

@interface NSArray (NSArrayCreation)
+ (id)arrayWithObjects:(const id [])objects count:(unsigned long)cnt;
@end

@interface NSDictionary
+ (id)dictionaryWithObjects:(const id [])objects forKeys:(const id [])keys count:(unsigned long)cnt;
@end

template<typename T>
void test_array_literals(T t) {
  id arr = @[ @17, t ]; // expected-error{{members of objective-c collection literals must be objects}}
}

template void test_array_literals(id);
template void test_array_literals(NSArray*);
template void test_array_literals(int); // expected-note{{in instantiation of function template specialization 'test_array_literals<int>' requested here}}

template<typename T>
void test_dictionary_literals(T t) {
  id dict = @{ @17 : t , t : @42 };
}

