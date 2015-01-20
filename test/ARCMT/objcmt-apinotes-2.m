// RUN: rm -rf %t/APINotesCache
// RUN: rm -rf %t
// RUN: %clang_cc1 -fblocks -mt-migrate-directory %t -objcmt-migrate-apinotes -objcmt-migrate-swift-unavailable -objcmt-migrate-designated-init -fapinotes -fapinotes-cache-path=%t/APINotesCache -I %S/../APINotes/Inputs/Headers -F %S/../APINotes/Inputs/Frameworks %s 
// RUN: c-arcmt-test -mt-migrate-directory %t | arcmt-test -verify-transformed-files %S/SomeKitForNullAnnotation.h.result
// rdar://18523118

#import <SomeKit/SomeKitForNullAnnotation.h>
