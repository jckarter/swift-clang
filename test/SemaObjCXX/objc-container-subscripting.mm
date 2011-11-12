// RUN: %clang_cc1 -triple x86_64-apple-darwin11 -fsyntax-only -verify %s

@class NSArray;

@interface NSMutableDictionary
- (id)objectForKeyedSubscript:(id)key;
- (void)setObject:(id)object forKeyedSubscript:(id)key;
@end

template<typename T, typename U, typename O>
void test_dictionary_subscripts(T base, U key, O obj) {
  base[key] = obj; // expected-error {{expected method to write array element not found on object of type 'NSMutableDictionary *'}} \
                   // expected-error {{assigning to 'id' from incompatible type 'int';}}
  obj = base[key];  // expected-error {{expected method to read array element not found on object of type 'NSMutableDictionary *'}} \
                    // expected-error {{assigning to 'int' from incompatible type 'id'; }}
     
}

template void test_dictionary_subscripts(NSMutableDictionary*, id, NSArray *ns);

template void test_dictionary_subscripts(NSMutableDictionary*, NSArray *ns, id);

template void test_dictionary_subscripts(NSMutableDictionary*, int, id); // expected-note {{in instantiation of function template specialization 'test_dictionary_subscripts<NSMutableDictionary *, int, id>' requested here}}

template void test_dictionary_subscripts(NSMutableDictionary*, id, int); // expected-note {{in instantiation of function template specialization 'test_dictionary_subscripts<NSMutableDictionary *, id, int>' requested here}}


@interface NSMutableArray
- (id)objectAtIndexedSubscript:(int)index;
- (void)objectAtIndexedSubscript:(int)index put:(id)object;
@end

template<typename T, typename U, typename O>
void test_array_subscripts(T base, U index, O obj) {
  base[index] = obj; // expected-error {{expected method to write array element not found on object of type 'NSMutableArray *'}}
  obj = base[index]; // expected-error {{expected method to read array element not found on object of type 'NSMutableArray *'}}
}

template void  test_array_subscripts(NSMutableArray *, int, id);
template void  test_array_subscripts(NSMutableArray *, short, id);
enum E { e };

template void  test_array_subscripts(NSMutableArray *, E, id);

template void  test_array_subscripts(NSMutableArray *, double, id); // expected-note {{in instantiation of function template specialization 'test_array_subscripts<NSMutableArray *, double, id>' requested here}}
