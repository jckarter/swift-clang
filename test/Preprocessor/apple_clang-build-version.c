// Check that __apple_build_version__ is defined.
//
// RUN: %clang -x c -dM -E /dev/null -o %t
// RUN: FileCheck < %t %s
//
// CHECK: __apple_build_version
//
// REQUIRES: apple-clang
