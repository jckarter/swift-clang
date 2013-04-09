// RUN: %clang -arch arm64 -miphoneos-version-min=6.0.0bar %s -verify 2>&1 | FileCheck -check-prefix=ARM64-INV %s
// RUN: %clang -arch arm64 -miphoneos-version-min=6.0.0 %s -verify 2>&1 | FileCheck -check-prefix=ARM64-BAD %s
// RUN: %clang -arch arm64 -miphoneos-version-min=7.0.0 %s -verify 2>&1| FileCheck -check-prefix=ARM64-GOOD %s

// ARM64-INV: error: invalid version number in '-miphoneos-version-min=6.0.0bar'
// ARM64-INV-NOT: invalid deployment target
// ARM64-BAD: invalid deployment target '-miphoneos-version-min=6.0.0' for architecture 'arm64' (requires '7.0.0' or later)
// ARM64-GOOD-NOT: invalid deployment target '-miphoneos-version-min=7.0.0' for architecture 'arm64' (requires '7.0.0' or later)
