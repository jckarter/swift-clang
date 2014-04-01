// RUN: %clang_cc1  -fsyntax-only -verify -Wno-objc-root-class %s
// rdar://16462586

__attribute__((objc_asm("MySecretNamespace.Protocol")))
@protocol Protocol
@end

__attribute__((objc_asm("MySecretNamespace.Message")))
@interface Message <Protocol> { 
__attribute__((objc_asm("MySecretNamespace.Message"))) // expected-error {{'objc_asm' attribute only applies to interface or protocol declarations}}
  id MyIVAR;
}
__attribute__((objc_asm("MySecretNamespace.Message")))
@property int MyProperty; // expected-error {{prefix attribute must be followed by an interface or protocol}}}}

- (int) getMyProperty __attribute__((objc_asm("MySecretNamespace.Message"))); // expected-error {{'objc_asm' attribute only applies to interface or protocol declarations}}

- (void) setMyProperty : (int) arg __attribute__((objc_asm("MySecretNamespace.Message"))); // expected-error {{'objc_asm' attribute only applies to interface or protocol declarations}}

@end

__attribute__((objc_asm("MySecretNamespace.ForwardClass")))
@class ForwardClass; // expected-error {{prefix attribute must be followed by an interface or protocol}}

__attribute__((objc_asm("MySecretNamespace.ForwardProtocol")))
@protocol ForwardProtocol;

__attribute__((objc_asm("MySecretNamespace.Message")))
@implementation Message // expected-error {{prefix attribute must be followed by an interface or protocol}}
__attribute__((objc_asm("MySecretNamespace.Message")))
- (id) MyMethod {
  return MyIVAR;
}
@end
