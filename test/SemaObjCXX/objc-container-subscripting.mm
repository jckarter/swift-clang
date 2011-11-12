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

