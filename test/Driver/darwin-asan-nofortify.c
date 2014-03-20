// Make sure AddressSanitizer disables _FORTIFY_SOURCE on Darwin.

// RUN: %clang -fsanitize=address  %s -E -dM -target x86_64-darwin | FileCheck %s

// rdar://11496765, rdar://12417750
// -faddress-sanitizer is not currently supported.
// XFAIL: *

// CHECK: #define _FORTIFY_SOURCE 0
