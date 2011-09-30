// RUN: %clang_cc1 -fsyntax-only -verify -std=c++0x -fblocks %s

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
struct ConvertibleTo {
  operator T();
};

template<typename T>
struct ExplicitlyConvertibleTo {
  explicit operator T();
};

void test_convertibility(ConvertibleTo<NSArray*> toArray,
                         ConvertibleTo<id> toId,
                         ConvertibleTo<int (^)(int)> toBlock,
                         ConvertibleTo<int> toInt,
                         ExplicitlyConvertibleTo<NSArray *> toArrayExplicit) {
  id array = @[ 
               toArray,
               toId,
               toBlock,
               toInt // expected-error{{collection element of type 'ConvertibleTo<int>' is not an Objective-C object}}
              ];
  id array2 = @[ toArrayExplicit ]; // expected-error{{collection element of type 'ExplicitlyConvertibleTo<NSArray *>' is not an Objective-C object}}
}

template<typename T>
void test_array_literals(T t) {
  id arr = @[ @17, t ]; // expected-error{{collection element of type 'int' is not an Objective-C object}}
}

template void test_array_literals(id);
template void test_array_literals(NSArray*);
template void test_array_literals(int); // expected-note{{in instantiation of function template specialization 'test_array_literals<int>' requested here}}

template<typename T, typename U>
void test_dictionary_literals(T t, U u) {
  id dict = @{ 
    @17 : t, // expected-error{{collection element of type 'int' is not an Objective-C object}}
    u : @42 // expected-error{{collection element of type 'int' is not an Objective-C object}}
  };
}

template void test_dictionary_literals(id, NSArray*);
template void test_dictionary_literals(NSArray*, id);
template void test_dictionary_literals(int, id); // expected-note{{in instantiation of function template specialization 'test_dictionary_literals<int, id>' requested here}}
template void test_dictionary_literals(id, int); // expected-note{{in instantiation of function template specialization 'test_dictionary_literals<id, int>' requested here}}

template<typename ...Args>
void test_bad_variadic_array_literal(Args ...args) {
  id arr1 = @[ args ]; // expected-error{{initializer contains unexpanded parameter pack 'args'}}
}

template<typename ...Args>
void test_variadic_array_literal(Args ...args) {
  id arr1 = @[ args... ]; // expected-error{{collection element of type 'int' is not an Objective-C object}}
}
template void test_variadic_array_literal(id);
template void test_variadic_array_literal(id, NSArray*);
template void test_variadic_array_literal(id, int, NSArray*); // expected-note{{in instantiation of function template specialization 'test_variadic_array_literal<id, int, NSArray *>' requested here}}

template<typename ...Args>
void test_bad_variadic_dictionary_literal(Args ...args) {
  id dict = @{ args : @17 }; // expected-error{{initializer contains unexpanded parameter pack 'args'}}
}

// FIXME: Handle key/value pairs that are pack expansions.
