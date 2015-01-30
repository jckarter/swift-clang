typedef __nonnull int* mynonnull;

__attribute__((objc_root_class))
@interface typedefClass
- (void) func1:(mynonnull)i;
@end

void func2(mynonnull i);

void func3(int *); // expected-warning{{pointer is missing a nullability type specifier (__nonnull or __nullable)}}

