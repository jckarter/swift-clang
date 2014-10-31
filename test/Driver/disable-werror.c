// RUN: env GCC_TREAT_WARNINGS_AS_ERRORS=NO %clang -target x86_64-apple-darwin %s -c -### 2>&1 | FileCheck %s
// RUN: env GCC_TREAT_WARNINGS_AS_ERRORS=YES %clang -target x86_64-apple-darwin %s -c -### 2>&1 | FileCheck -check-prefix=ALLOW_WERROR %s
// RUN: env GCC_TREAT_WARNINGS_AS_ERRORS=NO %clang -target x86_64-apple-darwin %s -fsyntax-only -lunused -Werror 2>&1 | FileCheck -check-prefix=WARN_UNUSED %s

// CHECK: "-cc1"
// CHECK: "-Wno-error"

// ALLOW_WERROR: "-cc1"
// ALLOW_WERROR-NOT: "-Wno-error"

// Test -Werror is also disabled for driver diagnostics.
// WARN_UNUSED: warning:
