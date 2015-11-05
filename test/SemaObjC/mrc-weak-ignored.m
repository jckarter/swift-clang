// RUN: %clang_cc1 -fobjc-runtime-has-weak -fignore-objc-weak -fsyntax-only -verify %s

__attribute__((objc_root_class))
@interface A
@property (weak) id wa;
@property (weak) id wb;
@property (weak) id wc; // expected-note {{property declared here}}
@property (weak) id wd; // expected-note {{property declared here}}
@property (unsafe_unretained) id ua;
@property (unsafe_unretained) id ub;
@property (unsafe_unretained) id uc;
@property (unsafe_unretained) id ud;
@property (strong) id sa;
@property (strong) id sb;
@property (strong) id sc; // expected-note {{property declared here}}
@property (strong) id sd;
@end

@implementation A {
  id _wa;
  __weak id _wb; // expected-warning {{ignoring __weak in file using manual reference counting}}
  __unsafe_unretained id _wc; // expected-error {{existing instance variable '_wc' for __weak property 'wc' must be __weak}}
  id _ua;
  __weak id _ub; // expected-warning {{ignoring __weak in file using manual reference counting}}
  __unsafe_unretained id _uc;
  id _sa;
  __weak id _sb; // expected-warning {{ignoring __weak in file using manual reference counting}}
  __unsafe_unretained id _sc; // expected-error {{existing instance variable '_sc' for strong property 'sc' may not be __unsafe_unretained}}
}
@synthesize wa = _wa;
@synthesize wb = _wb;
@synthesize wc = _wc; // expected-note {{property synthesized here}}
@synthesize wd = _wd; // expected-error {{cannot synthesize weak property in file using manual reference counting}}
@synthesize ua = _ua;
@synthesize ub = _ub;
@synthesize uc = _uc;
@synthesize ud = _ud;
@synthesize sa = _sa;
@synthesize sb = _sb;
@synthesize sc = _sc; // expected-note {{property synthesized here}}
@synthesize sd = _sd;
@end

void test_goto() {
  goto after;
  __weak id x; // expected-warning {{ignoring __weak in file using manual reference counting}}
after:
  return;
}

void test_weak_cast(id *value) {
  __weak id *a = (__weak id*) value; // expected-warning 2 {{ignoring __weak in file using manual reference counting}}
  id *b = (__weak id*) value; // expected-warning {{ignoring __weak in file using manual reference counting}}
  __weak id *c = (id*) value; // expected-warning {{ignoring __weak in file using manual reference counting}}
}

void test_unsafe_unretained_cast(id *value) {
  __unsafe_unretained id *a = (__unsafe_unretained id*) value;
  id *b = (__unsafe_unretained id*) value;
  __unsafe_unretained id *c = (id*) value;
}

void test_cast_qualifier_inference(__weak id *value) { // expected-warning {{ignoring __weak in file using manual reference counting}}
  __weak id *a = (id*) value; // expected-warning {{ignoring __weak in file using manual reference counting}}
  __unsafe_unretained id *b = (id*) value;
}

