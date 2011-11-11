// RUN: %clang_cc1 -emit-llvm -triple x86_64-apple-darwin %s -o /dev/null

typedef unsigned int size_t;
@protocol P @end

@interface NSMutableArray
- (id)objectAtIndexedSubscript:(size_t)index;
- (void)objectAtIndexedSubscript:(size_t)index put:(id)object;
@end

@interface XNSMutableArray
- (id)objectAtIndexedSubscript:(size_t)index;
- (void)objectAtIndexedSubscript:(size_t)index put:(id)object;
@end

int main() {
  NSMutableArray<P> * array;
  id oldObject = array[10];
 
  array[10] = oldObject;

  id unknown_array;
  oldObject = unknown_array[1];

  unknown_array[1] = oldObject;
}

