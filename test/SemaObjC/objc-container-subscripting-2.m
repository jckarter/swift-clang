// RUN: %clang_cc1  -fsyntax-only -verify %s

typedef unsigned int size_t;
@protocol P @end

@interface NSMutableArray
- (id)objectAtIndexedSubscript:(size_t)index;
- (void)objectAtIndexedSubscript:(size_t)index put:(id)object;
@end

@interface NSMutableDictionary
- (id)objectForKeyedSubscript:(id)key;
- (void)setObject:(id)object forKeyedSubscript:(size_t)key;
@end

void test_unused() {
  NSMutableArray *array;
  array[10]; // expected-warning {{container access result unused - container access should not be used for side effects}} 

  NSMutableDictionary *dict;
  dict[array]; // expected-warning {{container access result unused - container access should not be used for side effects}}
}

