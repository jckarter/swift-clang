// RUN: rm -rf %t
// RUN: %clang_cc1 -fsyntax-only -triple arm64-apple-ios -target-feature +neon -std=c99 -fmodules -fmodules-cache-path=%t -D__need_wint_t -ffreestanding -isysroot %S/Inputs/System/usr/include %s -verify
// expected-no-diagnostics

@import _Builtin_intrinsics.arm64.simd;
