// RUN: touch %t.o
// RUN: %clang -### -ccc-host-triple x86_64-apple-darwin10 -fobjc-link-runtime -mmacosx-version-min=10.7 %t.o 2>&1 | FileCheck -check-prefix=CHECK-ARCLITE-OSX %s
// RUN: %clang -### -ccc-host-triple x86_64-apple-darwin10 -fobjc-link-runtime -mmacosx-version-min=10.8 %t.o 2>&1 | FileCheck -check-prefix=CHECK-NOARCLITE %s

// CHECK-ARCLITE-OSX: libarclite_macosx.a
// CHECK-NOARCLITE-NOT: libarclite
