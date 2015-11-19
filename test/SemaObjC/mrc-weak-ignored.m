// RUN: %clang_cc1 -fobjc-runtime-has-weak -fignore-objc-weak -Werror -fsyntax-only -verify %s

__attribute__((objc_root_class))
@interface Root @end

@interface A : Root
@property (weak) id wa;
@property (weak) id wb;
@property (weak) id wc;
@property (weak) id wd; // expected-note {{property declared here}}
@property (unsafe_unretained) id ua;
@property (unsafe_unretained) id ub;
@property (unsafe_unretained) id uc;
@property (unsafe_unretained) id ud;
@property (strong) id sa;
@property (strong) id sb;
@property (strong) id sc;
@property (strong) id sd;
@end

@implementation A {
  id _wa;
  __weak id _wb; // expected-warning {{ignoring __weak in file using manual reference counting}}
  __unsafe_unretained id _wc;
  id _ua;
  __weak id _ub; // expected-warning {{ignoring __weak in file using manual reference counting}}
  __unsafe_unretained id _uc;
  id _sa;
  __weak id _sb; // expected-warning {{ignoring __weak in file using manual reference counting}}
  __unsafe_unretained id _sc;
}
@synthesize wa = _wa;
@synthesize wb = _wb;
@synthesize wc = _wc;
@synthesize wd = _wd; // expected-error {{cannot synthesize weak property in file using manual reference counting}}
@synthesize ua = _ua;
@synthesize ub = _ub;
@synthesize uc = _uc;
@synthesize ud = _ud;
@synthesize sa = _sa;
@synthesize sb = _sb;
@synthesize sc = _sc;
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

// No diagnostics here.
@interface UnusedDeclarations {
  __weak id _wi;
}
@property (readonly) __weak id wi;
@end

// The class implementation diagnoses __weak ivars.
@interface ImplementedWithWeakIvar : Root {
  __weak id _wi; // expected-warning {{ignoring __weak in file using manual reference counting}}
}
@end
@implementation ImplementedWithWeakIvar
@end

// Uses of __weak ivars are also diagnosed.
@interface PublicWeakIvar : Root {
@public
  __weak id _wi; // expected-note {{'_wi' declared here}}
}
@end

void test_public(PublicWeakIvar *p) {
  id x = p->_wi; // expected-warning {{'_wi' was declared with __weak, but __weak is ignored in files using manual reference counting}}
}

@interface SynthesizedWeakProperty : Root
@property (readonly) __weak id wi; // expected-warning {{ignoring __weak in file using manual reference counting}}
@end

@implementation SynthesizedWeakProperty
@end

@interface UsedWeakProperty : Root
@property (readonly) __weak id wi; // expected-note {{'wi' declared here}}
@end

void test_property(UsedWeakProperty *p) {
  id x = p.wi; // expected-warning {{'wi' was declared with __weak, but __weak is ignored in files using manual reference counting}}
}
