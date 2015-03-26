// RUN: %clang_cc1 -triple x86_64-unknown-unknown -fblocks -fobjc-runtime=macosx-fragile-10.5 -emit-llvm -o - %s

// Parameterized classes have no effect on code generation; this test
// mainly verifies that CodeGen doesn't assert when substituted types
// in uses of methods don't line up exactly with the parameterized
// types in the method declarations due to type erasure. "Not crash"
// is the only interesting criteria here.

@protocol NSObject
@end

@protocol NSCopying
@end

__attribute__((objc_root_class))
@interface NSObject <NSObject>
@end

@interface NSString : NSObject <NSCopying>
@end

@interface NSMutableArray<T> : NSObject <NSCopying>
@property (copy,nonatomic) T firstObject;
- (void)addObject:(T)object;
- (void)sortWithFunction:(int (*)(T, T))function;
- (void)getObjects:(T*)objects length:(unsigned*)length;
@end

NSString *getFirstObjectProp(NSMutableArray<NSString *> *array) {
  return array.firstObject;
}

NSString *getFirstObjectMethod(NSMutableArray<NSString *> *array) {
  return [array firstObject];
}

void addObject(NSMutableArray<NSString *> *array, NSString *obj) {
  [array addObject: obj];
}

int compareStrings(NSString *x, NSString *y) { return 0; }
int compareBlocks(NSString * (^x)(NSString *),
                  NSString * (^y)(NSString *)) { return 0; }

void sortTest(NSMutableArray<NSString *> *array,
              NSMutableArray<NSString * (^)(NSString *)> *array2) {
  [array sortWithFunction: &compareStrings];
  [array2 sortWithFunction: &compareBlocks];
}

void getObjectsTest(NSMutableArray<NSString *> *array) {
  NSString **objects;
  unsigned length;
  [array getObjects: objects length: &length];
}
