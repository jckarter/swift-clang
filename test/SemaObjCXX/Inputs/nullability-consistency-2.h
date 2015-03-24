void g1(__nonnull int *);

void g2(int (^block)(int, int)); // expected-warning{{block pointer is missing a nullability type specifier (__nonnull or __nullable)}}

void g3(const
        id // expected-warning{{missing a nullability type specifier}}
        volatile
        * // expected-warning{{missing a nullability type specifier}}
        ); 

@interface SomeClass {
  __nullable id ivar1;
  id ivar2;
}

@property (retain,nonnull) id property1;
@property (retain,nullable) SomeClass *property2;
- (nullable SomeClass *)method1;
- (void)method2:(nonnull SomeClass *)param;
@end