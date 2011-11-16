// RUN: %clang_cc1 -emit-llvm -triple x86_64-apple-darwin %s -o /dev/null

typedef unsigned int size_t;
@protocol P @end

@interface NSMutableArray
- (id)objectAtIndexedSubscript:(size_t)index;
- (void)setObject:(id)object atIndexedSubscript:(size_t)index;
@end

@interface XNSMutableArray
- (id)objectAtIndexedSubscript:(size_t)index;
- (void)setObject:(id)object atIndexedSubscript:(size_t)index;
@end

@interface NSMutableDictionary
- (id)objectForKeyedSubscript:(id)key;
- (void)setObject:(id)object forKeyedSubscript:(id)key;
@end

@class NSString;

int main() {
  NSMutableArray<P> * array;
  id oldObject = array[10];
 
  array[10] = oldObject;

  id unknown_array;
  oldObject = unknown_array[1];

  unknown_array[1] = oldObject;

  NSMutableDictionary *dictionary;
  NSString *key;
  id newObject;
  oldObject = dictionary[key];
  dictionary[key] = newObject;	// replace oldObject with newObject

}

