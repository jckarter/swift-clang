@interface NSMutableArray
- (id)objectAtIndexedSubscript:(int)index;
- (void)setObject:(id)object atIndexedSubscript:(int)index;
@end

@interface NSMutableDictionary
- (id)objectForKeyedSubscript:(id)key;
- (void)setObject:(id)object forKeyedSubscript:(id)key;
@end

@class NSString;

id testArray(int index) {
  NSMutableArray *array;
  array[3] = 0;
  return array[index];
}

void testDict() {
  NSMutableDictionary *dictionary;
  NSString *key;
  id newObject, oldObject;
  oldObject = dictionary[key];
  dictionary[key] = newObject;
}

// RUN: c-index-test -test-annotate-tokens=%s:13:1:25:1 %s | FileCheck %s
// CHECK: Identifier: "testArray" [13:4 - 13:13] FunctionDecl=testArray:13:4 (Definition)
// CHECK: Identifier: "array" [15:3 - 15:8] DeclRefExpr=array:14:19
// CHECK: Punctuation: "[" [15:8 - 15:9] UnexposedExpr=
// CHECK: Literal: "3" [15:9 - 15:10] IntegerLiteral=
// CHECK: Punctuation: "]" [15:10 - 15:11] UnexposedExpr=
// CHECK: Punctuation: "=" [15:12 - 15:13] BinaryOperator=
// CHECK: Literal: "0" [15:14 - 15:15] IntegerLiteral=
// CHECK: Punctuation: ";" [15:15 - 15:16] CompoundStmt=
// CHECK: Keyword: "return" [16:3 - 16:9] ReturnStmt=
// CHECK: Identifier: "array" [16:10 - 16:15] DeclRefExpr=array:14:19
// CHECK: Punctuation: "[" [16:15 - 16:16] UnexposedExpr=
// CHECK: Identifier: "index" [16:16 - 16:21] DeclRefExpr=index:13:18
// CHECK: Punctuation: "]" [16:21 - 16:22] UnexposedExpr=
// CHECK: Punctuation: ";" [16:22 - 16:23] CompoundStmt=

// CHECK: Identifier: "testDict" [19:6 - 19:14] FunctionDecl=testDict:19:6 (Definition)
// CHECK: Identifier: "oldObject" [23:3 - 23:12] DeclRefExpr=oldObject:22:17
// CHECK: Punctuation: "=" [23:13 - 23:14] BinaryOperator=
// CHECK: Identifier: "dictionary" [23:15 - 23:25] DeclRefExpr=dictionary:20:24
// CHECK: Punctuation: "[" [23:25 - 23:26] UnexposedExpr=
// CHECK: Identifier: "key" [23:26 - 23:29] DeclRefExpr=key:21:13
// CHECK: Punctuation: "]" [23:29 - 23:30] UnexposedExpr=
// CHECK: Punctuation: ";" [23:30 - 23:31] CompoundStmt=
// CHECK: Identifier: "dictionary" [24:3 - 24:13] DeclRefExpr=dictionary:20:24
// CHECK: Punctuation: "[" [24:13 - 24:14] UnexposedExpr=
// CHECK: Identifier: "key" [24:14 - 24:17] DeclRefExpr=key:21:13
// CHECK: Punctuation: "]" [24:17 - 24:18] UnexposedExpr=
// CHECK: Punctuation: "=" [24:19 - 24:20] BinaryOperator=
// CHECK: Identifier: "newObject" [24:21 - 24:30] DeclRefExpr=newObject:22:6
// CHECK: Punctuation: ";" [24:30 - 24:31] CompoundStmt=
