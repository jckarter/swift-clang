// RUN: %clang -target x86_64-apple-darwin -stdlib=libstdc++ -lstdc++ -mmacosx-version-min=10.9 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-DEPRECATED
// RUN: %clang -target x86_64-apple-darwin -stdlib=libstdc++ -lstdc++ -mmacosx-version-min=10.8 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-WITH-OSX-VERSION
// RUN: %clang -target x86_64-apple-darwin -stdlib=libc++ -lstdc++ -mmacosx-version-min=10.9 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SUPPORTED

// RUN: %clang -target x86_64-apple-darwin -arch armv7 -stdlib=libstdc++ -lstdc++ -mios-version-min=7.0 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-DEPRECATED
// RUN: %clang -target x86_64-apple-darwin -arch armv7 -stdlib=libstdc++ -lstdc++ -mios-version-min=6.0 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-WITH-IOS-VERSION
// RUN: %clang -target x86_64-apple-darwin -arch armv7 -stdlib=libc++ -lstdc++ -mios-version-min=7.0 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SUPPORTED

// CHECK-SUPPORTED-NOT: libstdc++ is deprecated
// CHECK-DEPRECATED: libstdc++ is deprecated; move to libc++
// CHECK-DEPRECATED-NOT: minimum deployment target
// CHECK-WITH-IOS-VERSION: libstdc++ is deprecated; move to libc++ with a minimum deployment target of iOS 7
// CHECK-WITH-OSX-VERSION: libstdc++ is deprecated; move to libc++ with a minimum deployment target of OS X 10.9
