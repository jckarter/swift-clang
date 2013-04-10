// RUN: %clang -arch x86_64 -mios-simulator-version-min=6.0.0bar %s -verify 2>&1 | FileCheck -check-prefix=X86_64-INV %s
// RUN: %clang -arch x86_64 -mios-simulator-version-min=6.0.0 %s -verify 2>&1 | FileCheck -check-prefix=X86_64-BAD %s
// RUN: %clang -arch x86_64 -mios-simulator-version-min=7.0.0 %s -verify 2>&1| FileCheck -check-prefix=X86_64-GOOD %s

// X86_64-INV: error: invalid version number in '-mios-simulator-version-min=6.0.0bar'
// X86_64-INV-NOT: invalid deployment target
// X86_64-BAD: invalid deployment target '-mios-simulator-version-min=6.0.0' for architecture 'x86_64' (requires '7.0.0' or later)
// X86_64-GOOD-NOT: invalid deployment target '-mios-simulator-version-min=7.0.0' for architecture 'x86_64' (requires '7.0.0' or later)
