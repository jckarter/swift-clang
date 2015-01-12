// RUN: %clang -ccc-print-bindings -c %s -fembed-bitcode 2>&1 | FileCheck %s
// CHECK: clang
// CHECK: clang

// RUN: %clang -c %s -fembed-bitcode 2>&1 -### | FileCheck %s -check-prefix=CHECK-CC
// CHECK-CC: -cc1
// CHECK-CC: -emit-llvm-bc
// CHECK-CC: -cc1
// CHECK-CC: -emit-obj
// CHECK-CC: -fembed-bitcode
