// RUN: %clang_cc1 -verify -fsyntax-only -fobjc-arc -fblocks %s

// --- swift_private ---

__attribute__((swift_private))
@protocol FooProto
@end

__attribute__((swift_private))
@interface Foo
@end

@interface Bar
@property id prop __attribute__((swift_private));
- (void)instMethod __attribute__((swift_private));
+ (instancetype)bar __attribute__((swift_private));
@end

void function(id) __attribute__((swift_private));

struct __attribute__((swift_private)) Point {
  int x;
  int y;
};

enum __attribute__((swift_private)) Colors {
  Red, Green, Blue
};

typedef struct {
  float x, y, z;
} Point3D __attribute__((swift_private));


// --- swift_name ---

__attribute__((swift_name("SNFooType")))
@protocol SNFoo
@end

__attribute__((swift_name("SNFooClass")))
@interface SNFoo <SNFoo>
- (instancetype)init __attribute__((swift_name("init"))); // expected-error {{only factory methods can have the 'swift_name' attribute}}
- (instancetype)initWithValue:(int)value __attribute__((swift_name("fooWithValue(_:)"))); // expected-error {{only factory methods can have the 'swift_name' attribute}}

+ (void)refresh __attribute__((swift_name("refresh()"))); // expected-error {{only factory methods can have the 'swift_name' attribute}}

+ (instancetype)foo __attribute__((swift_name("foo()")));
+ (SNFoo *)fooWithValue:(int)value __attribute__((swift_name("foo(value:)")));
+ (SNFoo *)fooWithValue:(int)value value:(int)value2 __attribute__((swift_name("foo(value:extra:)")));
+ (SNFoo *)fooWithConvertingValue:(int)value value:(int)value2 __attribute__((swift_name("init(_:extra:)")));

+ (SNFoo *)fooWithOtherValue:(int)value __attribute__((swift_name("init"))); // expected-error {{parameter of 'swift_name' attribute must be a Swift method name string}}
+ (SNFoo *)fooWithAnotherValue:(int)value __attribute__((swift_name("foo()"))); // expected-warning {{too few parameters in 'swift_name' attribute (expected 1; got 0)}}
+ (SNFoo *)fooWithYetAnotherValue:(int)value __attribute__((swift_name("foo(value:extra:)"))); // expected-warning {{too many parameters in 'swift_name' attribute (expected 1; got 2)}}

+ (SNFoo *)fooAndReturnErrorCode:(int *)errorCode __attribute__((swift_name("foo()"))); // no-warning
+ (SNFoo *)fooWithValue:(int)value andReturnErrorCode:(int *)errorCode __attribute__((swift_name("foo(value:)"))); // no-warning
+ (SNFoo *)fooFromErrorCode:(const int *)errorCode __attribute__((swift_name("foo()"))); // expected-warning {{too few parameters in 'swift_name' attribute (expected 1; got 0)}}
+ (SNFoo *)fooWithValue:(int)value fromErrorCode:(const int *)errorCode __attribute__((swift_name("foo(value:)"))); // expected-warning {{too few parameters in 'swift_name' attribute (expected 2; got 1)}}
+ (SNFoo *)fooWithPointerA:(int *)value andReturnErrorCode:(int *)errorCode __attribute__((swift_name("foo()"))); // no-warning
+ (SNFoo *)fooWithPointerB:(int *)value andReturnErrorCode:(int *)errorCode __attribute__((swift_name("foo(pointer:)"))); // no-warning
+ (SNFoo *)fooWithPointerC:(int *)value andReturnErrorCode:(int *)errorCode __attribute__((swift_name("foo(pointer:errorCode:)"))); // no-warning
+ (SNFoo *)fooWithOtherFoo:(SNFoo *)other __attribute__((swift_name("foo()"))); // expected-warning {{too few parameters in 'swift_name' attribute (expected 1; got 0)}}

+ (instancetype)specialFoo __attribute__((swift_name("init(options:)")));
+ (instancetype)specialBar __attribute__((swift_name("init(options:extra:)"))); // expected-warning {{too many parameters in 'swift_name' attribute (expected 0; got 2)}}
+ (instancetype)specialBaz __attribute__((swift_name("init(_:)"))); // expected-warning {{too many parameters in 'swift_name' attribute (expected 0; got 1)}}
+ (instancetype)specialGarply __attribute__((swift_name("foo(options:)"))); // expected-warning {{too many parameters in 'swift_name' attribute (expected 0; got 1)}}

+ (instancetype)trailingParen __attribute__((swift_name("foo("))); // expected-error {{parameter of 'swift_name' attribute must be a Swift method name string}}
+ (instancetype)trailingColon:(int)value __attribute__((swift_name("foo(value)"))); // expected-error {{parameter of 'swift_name' attribute must be a Swift method name string}}
+ (instancetype)initialIgnore:(int)value __attribute__((swift_name("_(value:)"))); // expected-error {{parameter of 'swift_name' attribute must be a Swift method name string}}
+ (instancetype)middleOmitted:(int)value __attribute__((swift_name("foo(:)"))); // expected-error {{parameter of 'swift_name' attribute must be a Swift method name string}}

@property(strong) id someProp __attribute__((swift_name("prop"))); // expected-error {{'swift_name' attribute cannot be applied to this declaration}}
@end

enum __attribute__((swift_name("MoreColors"))) MoreColors { // expected-error {{'swift_name' attribute cannot be applied to this declaration}}
  Cyan,
  Magenta,
  Yellow __attribute__((swift_name("RoseGold"))),
  Black __attribute__((swift_name("SpaceGrey()"))) // expected-error {{parameter of 'swift_name' attribute must be an ASCII identifier string}}
};
