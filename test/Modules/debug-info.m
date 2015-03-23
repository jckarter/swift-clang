// Modules:
// RUN: rm -rf %t
// RUN: %clang_cc1 -fmodules -DMODULES -fmodules-cache-path=%t %s -I %S/Inputs -I %t -emit-llvm -o %t.o
// RUN: llvm-dwarfdump %t/*/MethodPoolA-*.pcm | FileCheck %s --check-prefix=CHECK-MODULE
// PCH:
// RUN: %clang_cc1 -x objective-c -emit-pch -I %S/Inputs -o %t.pch %S/Inputs/MethodPoolA.h
// RUN: llvm-dwarfdump %t.pch | FileCheck %s --check-prefix=CHECK-MODULE

#ifdef MODULES
@import MethodPoolA;
#endif

// CHECK-MODULE: DW_TAG_structure_type
// CHECK-MODULE: DW_AT_name {{.*}}"A"
// CHECK-MODULE: DW_TAG_subprogram
// CHECK-MODULE: DW_AT_name {{.*}}"-[A method2:]"

int foo(A *a) {
  return [a method2: 0];
}
