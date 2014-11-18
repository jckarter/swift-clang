void g1(__nonnull int *);

void g2(int (^block)(int, int)); // expected-warning{{block pointer is missing a nullability type specifier (__nonnull, __nullable, or __null_unspecified)}}
