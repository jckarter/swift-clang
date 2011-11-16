// RUN: %clang_cc1  -fsyntax-only -verify %s

typedef unsigned int size_t;
@protocol P @end

@interface NSMutableArray
- (id)objectAtIndexedSubscript:(double)index; // expected-note {{parameter of type 'double' is declared here}}
- (void)setObject:(id *)object atIndexedSubscript:(void *)index; // expected-note {{parameter of type 'void *' is declared here}} \
								 // expected-note {{parameter of type 'id *' is declared here}}
@end
@interface I @end

int main() {
  NSMutableArray<P> * array;
  id  oldObject = array[10]; // expected-error {{type of the method's index parameter is not integral type}}
  array[3] = 0; // expected-error {{type of the method's index parameter is not integral type}} \
                // expected-error {{cannot assign to this array because assigning method's 2nd parameter of type 'id *' is not objective-c pointer type}}

  I* iarray;
  iarray[3] = 0; // expected-error {{expected method to write array element not found on object of type 'I *'}}
  I* p = iarray[4]; // expected-error {{expected method to read array element not found on object of type 'I *'}}

  oldObject = array[10]++; // expected-error {{illegal operation on objective-c container subscripting}}
  oldObject = array[10]--; // expected-error {{illegal operation on objective-c container subscripting}}
  oldObject = --array[10]; // expected-error {{illegal operation on objective-c container subscripting}}
}

@interface NSMutableDictionary
- (id)objectForKeyedSubscript:(id*)key; // expected-note {{parameter of type 'id *' is declared here}}
- (void)setObject:(void*)object forKeyedSubscript:(id*)key; // expected-note {{parameter of type 'void *' is declared here}} \
                                                            // expected-note {{parameter of type 'id *' is declared here}}
@end
@class NSString;

void testDict() {
  NSMutableDictionary *dictionary;
  NSString *key;
  id newObject, oldObject;
  oldObject = dictionary[key];  // expected-error {{type of the method's key parameter is not object type}}
  dictionary[key] = newObject;  // expected-error {{type of the method's key parameter is not object type}} \
                                // expected-error {{type of dictionary method's object parameter is not object type}}
}
