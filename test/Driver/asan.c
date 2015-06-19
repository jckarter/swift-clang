// RUN: %clang     -target i386-unknown-linux -fsanitize=address %s -S -emit-llvm -o - | FileCheck %s --check-prefix=CHECK-ASAN
// RUN: %clang -O1 -target i386-unknown-linux -fsanitize=address %s -S -emit-llvm -o - | FileCheck %s --check-prefix=CHECK-ASAN
// RUN: %clang -O2 -target i386-unknown-linux -fsanitize=address %s -S -emit-llvm -o - | FileCheck %s --check-prefix=CHECK-ASAN
// RUN: %clang -O3 -target i386-unknown-linux -fsanitize=address %s -S -emit-llvm -o - | FileCheck %s --check-prefix=CHECK-ASAN
// RUN: not %clang     -target i386-unknown-linux -fsanitize=kernel-address %s -S -emit-llvm -o - 2>&1 | FileCheck %s --check-prefix=CHECK-KASAN
// RUN: not %clang -O1 -target i386-unknown-linux -fsanitize=kernel-address %s -S -emit-llvm -o - 2>&1 | FileCheck %s --check-prefix=CHECK-KASAN
// RUN: not %clang -O2 -target i386-unknown-linux -fsanitize=kernel-address %s -S -emit-llvm -o - 2>&1 | FileCheck %s --check-prefix=CHECK-KASAN
// RUN: not %clang -O3 -target i386-unknown-linux -fsanitize=kernel-address %s -S -emit-llvm -o - 2>&1 | FileCheck %s --check-prefix=CHECK-KASAN
// Verify that -fsanitize={address,kernel-address} invoke ASan and KASan instrumentation.

int foo(int *a) { return *a; }
// CHECK-ASAN: __asan_init
// CHECK-KASAN: unsupported argument 'kernel-address' to option 'fsanitize='
