// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -emit-llvm  -o /dev/null %s

typedef unsigned int size_t;
@protocol P @end

@interface NSMutableArray
- (id)objectAtIndexedSubscript:(size_t)index;
- (void)setObject:(id)object atIndexedSubscript:(size_t)index;
@end

struct S {
  operator unsigned int ();
  operator id ();
};

@interface NSMutableDictionary
- (id)objectForKeyedSubscript:(id)key;
- (void)setObject:(id)object forKeyedSubscript:(id)key;
@end

int main() {
  NSMutableArray<P> * array;
  S s;
  id oldObject = array[s];

  NSMutableDictionary<P> *dict;
  dict[(id)s] = oldObject;
  oldObject = dict[(id)s];

}
