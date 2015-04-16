// RUN: %clang -target x86_64-apple-macosx -arch armv7k -x c -E -dM %s -o - | FileCheck --check-prefix=CHECK-V7K %s
// CHECK-V7K: #define __BIGGEST_ALIGNMENT__ 8

// RUN: %clang -target x86_64-apple-macosx -arch armv7m -x c -E -dM %s -o - | FileCheck --check-prefix=CHECK-AAPCS %s
// CHECK-AAPCS: #define __BIGGEST_ALIGNMENT__ 8

// RUN: %clang -target x86_64-apple-macosx -arch armv7s -x c -E -dM %s -o - | FileCheck --check-prefix=CHECK-APCS %s
// CHECK-APCS: #define __BIGGEST_ALIGNMENT__ 4
