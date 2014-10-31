// RUN: rm -rf %t/APINotesCache
// RUN: rm -rf %t
// RUN: %clang_cc1 -mt-migrate-directory %t -objcmt-migrate-apinotes -fapinotes -fapinotes-cache-path=%t/APINotesCache -I %S/../APINotes/Inputs/Headers -F %S/../APINotes/Inputs/Frameworks %s 
// RUN: c-arcmt-test -mt-migrate-directory %t | arcmt-test -verify-transformed-files %S/SomeKit.h.result
// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -fsyntax-only -x objective-c %S/SomeKit.h.result
// rdar://18523118

#import <SomeKit/SomeKit.h>
