// RUN: touch %t.o
// RUN: %clang -### -target x86_64-apple-darwin10 -fobjc-link-runtime -lfoo -mmacosx-version-min=10.7 %t.o 2>&1 | FileCheck -check-prefix=CHECK-ARCLITE-OSX %s
// RUN: %clang -### -target x86_64-apple-darwin10 -fobjc-link-runtime -mmacosx-version-min=10.8 %t.o 2>&1 | FileCheck -check-prefix=CHECK-NOARCLITE %s
// RUN: %clang -### -target i386-apple-darwin10 -fobjc-link-runtime -mmacosx-version-min=10.7 %t.o 2>&1 | FileCheck -check-prefix=CHECK-NOARCLITE %s
// RUN: %clang -### -target x86_64-apple-darwin10 -fobjc-link-runtime -nostdlib %t.o 2>&1 | FileCheck -check-prefix=CHECK-NOSTDLIB %s

// CHECK-ARCLITE-OSX: -lfoo
// CHECK-ARCLITE-OSX: libarclite_macosx.a
// CHECK-ARCLITE-OSX: -framework
// CHECK-ARCLITE-OSX: Foundation
// CHECK-ARCLITE-OSX: -lobjc
// CHECK-NOARCLITE-NOT: libarclite
// CHECK-NOSTDLIB-NOT: -lobjc

// RUN: %clang -### -target x86_64-apple-darwin10 -fobjc-link-runtime -fobjc-arc -mmacosx-version-min=10.7 %s 2>&1 | FileCheck -check-prefix=CHECK-UNUSED %s

// CHECK-UNUSED-NOT: warning: argument unused during compilation: '-fobjc-link-runtime'

// RUN: %clang -### -target arm64-apple-ios -fobjc-link-runtime -lfoo -mios-version-min=4.0 %t.o 2>&1 | FileCheck -check-prefix=CHECK-ARCLITE-IOS %s
// CHECK-ARCLITE-IOS: libarclite_iphoneos.a

// RUN: %clang -### -target x86_64-apple-ios -fobjc-link-runtime -mios-version-min=4.0 %t.o 2>&1 | FileCheck -check-prefix=CHECK-ARCLITE-IOSSIM %s
// CHECK-ARCLITE-IOSSIM: libarclite_iphonesimulator.a

// RUN: %clang -### -target armv7-apple-tvos -fobjc-link-runtime -mtvos-version-min=4.0 %t.o 2>&1 | FileCheck -check-prefix=CHECK-ARCLITE-TVOS %s
// CHECK-ARCLITE-TVOS: libarclite_appletvos.a

// RUN: %clang -### -target i386-apple-tvos -fobjc-link-runtime -mtvos-version-min=4.0 %t.o 2>&1 | FileCheck -check-prefix=CHECK-ARCLITE-TVSIM %s
// CHECK-ARCLITE-TVSIM: libarclite_appletvsimulator.a
