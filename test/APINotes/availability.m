// RUN: %clang_cc1 -fapinotes -fapinotes-cache-path=%t/APINotesCache -fsyntax-only -I %S/Inputs/Headers -F %S/Inputs/Frameworks %s -verify

#include "HeaderLib.h"
#import <SomeKit/SomeKit.h>
#import <SomeKit/SomeKit_Private.h>

int main() {
  int i;
  i = unavailable_function(); // expected-error{{'unavailable_function' is unavailable: I beg you not to use this}}
  // expected-note@HeaderLib.h:8{{'unavailable_function' has been explicitly marked unavailable here}}
  i = unavailable_global_int; // expected-error{{'unavailable_global_int' is unavailable}}
  // expected-note@HeaderLib.h:9{{'unavailable_global_int' has been explicitly marked unavailable here}}

  B *b = 0; // expected-error{{'B' is unavailable: just don't}}
  // expected-note@SomeKit/SomeKit.h:15{{'B' has been explicitly marked unavailable here}}

  id<InternalProtocol> proto = 0; // expected-error{{'InternalProtocol' is unavailable: not for you}}
  // expected-note@SomeKit/SomeKit_Private.h:12{{'InternalProtocol' has been explicitly marked unavailable here}}

  A *a = 0;
  i = a.intValue; // expected-error{{intValue' is unavailable: wouldn't work anyway}}
  // expected-note@SomeKit/SomeKit.h:12{{'intValue' has been explicitly marked unavailable here}}

  [a transform:a]; // expected-error{{'transform:' is unavailable: anything but this}}
  // expected-note@SomeKit/SomeKit.h:6{{'transform:' has been explicitly marked unavailable here}}

  return 0;
}

