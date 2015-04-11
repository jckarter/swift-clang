
@class F;

@interface A
+ (int)method1;
- (int)method2:(int)param;
@end

@interface B : A
@end

@interface C
@end

enum {
  ENUM_VALUE = 23
};

enum {
  ENUM_VAL2 = 42
};

typedef enum {
  ENUM_VAL3 = 3
} enum3;
