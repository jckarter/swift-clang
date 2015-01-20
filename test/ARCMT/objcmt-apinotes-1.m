// RUN: rm -rf %t/APINotesCache
// RUN: rm -rf %t
// RUN: %clang_cc1 -mt-migrate-directory %t -objcmt-migrate-apinotes -objcmt-migrate-swift-unavailable -fapinotes -fapinotes-cache-path=%t/APINotesCache -I %S/../APINotes/Inputs/Headers -F %S/../APINotes/Inputs/Frameworks %s
// RUN: c-arcmt-test -mt-migrate-directory %t | arcmt-test -verify-transformed-files %S/HeaderLib.h.result
// rdar://18523118

#include "HeaderLib.h"
