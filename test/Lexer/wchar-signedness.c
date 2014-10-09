// RUN: %clang_cc1 -fsyntax-only -dM -E %s -triple x86_64-none-linux-gnu | FileCheck %s --check-prefix=CHECK-X86
// RUN: %clang_cc1 -fsyntax-only -dM -E %s -triple armv7-none-eabi | FileCheck %s --check-prefix=CHECK-ARM

// RUN: %clang_cc1 -fsyntax-only -dM -E %s -triple armv7-apple-ios | FileCheck %s --check-prefix=CHECK-IOS
// RUN: %clang_cc1 -fsyntax-only -dM -E %s -triple armv7k-apple-ios-eabi | FileCheck %s --check-prefix=CHECK-IOS
// rdar://15850244 - On iOS wchar_t is signed int even for AAPCS.

// CHECK-X86-NOT: #define __WCHAR_UNSIGNED__
// CHECK-X86: #define __WINT_UNSIGNED__ 1

// CHECK-ARM: #define __WCHAR_UNSIGNED__ 1
// CHECK-ARM-NOT: #define __WINT_UNSIGNED__ 1

// CHECK-IOS-NOT: #define __WCHAR_UNSIGNED__
