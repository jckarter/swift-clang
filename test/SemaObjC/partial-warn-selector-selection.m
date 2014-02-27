// RUN: %clang_cc1 -fsyntax-only -verify %s
@partial_interface Object ;
@partial_interface Class1;

__attribute((objc_complete_definition))
@interface Object 
- (void)foo;
@end

__attribute((objc_complete_definition))
@interface Class1
- (void)setWindow:(Object *)wdw;
@end

void foo(void) {
  Object *obj;
  [obj setWindow:0]; // expected-warning{{'Object' may not respond to 'setWindow:'}}
}
