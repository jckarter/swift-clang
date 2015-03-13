// RUN: %clang -target x86_64-apple-darwin10 \
// RUN:   -mkernel -### -fsyntax-only %s 2> %t
// RUN: FileCheck --check-prefix=CHECK-X86 < %t %s

// CHECK-X86: "-disable-red-zone"
// CHECK-X86: "-fno-builtin"
// CHECK-X86: "-fno-rtti"
// CHECK-X86: "-fno-common"

// RUN: %clang -target x86_64-apple-darwin10 \
// RUN:   -arch armv7 -mkernel -mstrict-align -### -fsyntax-only %s 2> %t
// RUN: FileCheck --check-prefix=CHECK-ARM < %t %s

// CHECK-ARM: "-backend-option" "-arm-long-calls"
// CHECK-ARM: "-backend-option" "-arm-strict-align"
// CHECK-ARM-NOT: "-backend-option" "-arm-strict-align"
// CHECK-ARM: "-fno-builtin"
// CHECK-ARM: "-fno-rtti"
// CHECK-ARM: "-fno-common"

// RUN: %clang -target x86_64-apple-darwin10 \
// RUN:   -Werror -fno-builtin -fno-exceptions -fno-common -fno-rtti \
// RUN:   -mkernel -fsyntax-only %s

// RUN: %clang -c %s -target armv7k-apple-watchos -fapple-kext -mwatchos-version-min=1.0.0 -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CHECK-WATCH
// CHECK-WATCH-NOT: "-backend-option" "-arm-long-calls"
