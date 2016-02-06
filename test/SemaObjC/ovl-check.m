// RUN: %clang_cc1 -triple x86_64-apple-darwin11 -fsyntax-only -verify %s -fobjc-arc
//
// These tests exist as a means to help ensure that diagnostics aren't printed
// in overload resolution in ObjC.

struct Type1 { int a; };
@interface Iface1 @end

@interface NeverCalled
- (void) test:(struct Type1 *)arg;
@end

@interface TakesIface1
- (void) test:(Iface1 *)arg;
@end

// PR26085, rdar://problem/24111333
void testTakesIface1(id x, Iface1 *arg) {
  // This should resolve silently to `TakesIface1`.
  [x test:arg];
}

@class NSString;
@interface NeverCalledv2
- (void) testStr:(NSString *)arg;
@end

@interface TakesVanillaConstChar
- (void) testStr:(const void *)a;
@end

// Not called out explicitly by PR26085, but related.
void testTakesNSString(id x) {
  // Overload resolution should not emit a diagnostic about needing to add an
  // '@' before "someStringLiteral".
  [x testStr:"someStringLiteral"];
}

typedef const void *CFTypeRef;
id CreateSomething();

@interface NeverCalledv3
- (void) testCFTypeRef:(struct Type1 *)arg;
@end

@interface TakesCFTypeRef
- (void) testCFTypeRef:(CFTypeRef)arg;
@end

// expected-no-diagnostics
