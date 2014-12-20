// RUN: %clang_cc1 -triple x86_64-unknown-unknown -fobjc-runtime=macosx-fragile-10.5 -emit-llvm -o - %s

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
