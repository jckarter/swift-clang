// RUN: touch %t.o
// RUN: %clang -### -ccc-host-triple x86_64-apple-darwin10 -fobjc-link-runtime -mmacosx-version-min=10.7 %t.o 2>&1 | FileCheck -check-prefix=CHECK-ARCLITE-OSX %s
// RUN: %clang -### -ccc-host-triple x86_64-apple-darwin10 -fobjc-link-runtime -mmacosx-version-min=10.8 %t.o 2>&1 | FileCheck -check-prefix=CHECK-NOARCLITE %s
// RUN: %clang -### -ccc-host-triple i386-apple-darwin10 -fobjc-link-runtime -arch i386 -mmacosx-version-min=10.6 -D__IPHONE_OS_VERSION_MIN_REQUIRED=50100 %t.o 2>&1 | FileCheck -check-prefix=CHECK-ARCLITE-SIM %s
// RUN: %clang -### -ccc-host-triple i386-apple-darwin10 -fobjc-link-runtime -arch i386 -mmacosx-version-min=10.6 -D__IPHONE_OS_VERSION_MIN_REQUIRED=60000 %t.o 2>&1 | FileCheck -check-prefix=CHECK-NOARCLITE %s
// RUN: %clang -### -ccc-host-triple i386-apple-darwin10 -fobjc-link-runtime -arch i386 -mios-simulator-version-min=5.1 %t.o 2>&1 | FileCheck -check-prefix=CHECK-ARCLITE-SIM %s
// RUN: %clang -### -ccc-host-triple i386-apple-darwin10 -fobjc-link-runtime -arch i386 -mios-simulator-version-min=6.0 %t.o 2>&1 | FileCheck -check-prefix=CHECK-NOARCLITE %s
// RUN: %clang -### -ccc-host-triple i386-apple-darwin10 -fobjc-link-runtime -arch armv6 -miphoneos-version-min=5.1 %t.o 2>&1 | FileCheck -check-prefix=CHECK-ARCLITE-IOS %s
// RUN: %clang -### -ccc-host-triple i386-apple-darwin10 -fobjc-link-runtime -arch armv6 -miphoneos-version-min=6.0 %t.o 2>&1 | FileCheck -check-prefix=CHECK-NOARCLITE %s

// CHECK-ARCLITE-OSX: libarclite_macosx.a
// CHECK-ARCLITE-SIM: libarclite_iphonesimulator.a
// CHECK-ARCLITE-IOS: libarclite_iphoneos.a
// CHECK-NOARCLITE-NOT: libarclite
