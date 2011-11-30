// RUN: %clang_cc1 -fobjc-arc -emit-llvm -triple x86_64-apple-darwin -o - %s | FileCheck %s

@interface NSMutableArray
- (id)objectAtIndexedSubscript:(int)index;
- (void)setObject:(id)object atIndexedSubscript:(int)index;
@end

id func() {
  NSMutableArray *array;
  array[3] = 0;
  return array[3];
}

// CHECK: [[call:%.*]] = call i8* bitcast (i8* (i8*, i8*, ...)* @objc_msgSend
// CHECK: [[SIX:%.*]] = call i8* @objc_retainAutoreleasedReturnValue(i8* [[call]]) nounwind
// CHECK: store i8* [[SIX]], i8** [[RETVAL:%.*]]
// CHECK: [[EIGHT:%.*]] = load i8** [[RETVAL]]
// CHECK: call i8* @objc_autoreleaseReturnValue(i8* [[EIGHT]]) nounwind

