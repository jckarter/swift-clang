// RUN: rm -rf %t

// Modules:
// RUN: %clang_cc1 -g -dwarf-ext-refs -fmodules -DMODULES -fmodules-cache-path=%t %s -I %S/Inputs -I %t -emit-llvm -o %t-mod.ll
// RUN: llvm-dwarfdump %t/*/DebugModule-*.pcm | FileCheck %s --check-prefix=CHECK-AST
// RUN: cat %t-mod.ll |  FileCheck %s --check-prefix=CHECK-EXTREF

// PCH:
// RUN: %clang_cc1 -x objective-c -emit-pch -I %S/Inputs -o %t.pcm %S/Inputs/DebugModule.h
// RUN: llvm-dwarfdump %t.pcm | FileCheck %s --check-prefix=CHECK-AST
// RUN: %clang_cc1 -g -dwarf-ext-refs -include-pch %t.pcm %s -emit-llvm -o %t-pch.ll %s
// RUN: cat %t-pch.ll |  FileCheck %s --check-prefix=CHECK-EXTREF


#ifdef MODULES
@import DebugModule;
#endif

@interface F
@end

// CHECK-AST: DW_TAG_compile_unit
// CHECK-AST: DW_TAG_structure_type
// CHECK-AST: DW_AT_name {{.*}}"A"
// CHECK-AST: DW_TAG_subprogram
// CHECK-AST: DW_AT_name {{.*}}"-[A method2:]"

// CHECK-EXTREF: !MDDerivedType(tag: DW_TAG_pointer_type, baseType: ![[A:[0-9]+]],
// CHECK-EXTREF: ![[A]] = !MDExternalTypeRef(tag: DW_TAG_structure_type, file: ![[PCM:[0-9]+]], identifier: "c:objc(cs)A")
// CHECK-EXTREF: ![[PCM]] = !MDFile(filename: "{{.*}}.pcm", directory: "")
// CHECK-EXTREF: !MDCompositeType(tag: DW_TAG_structure_type, name: "F"
int foo(A *a, F *f) {
  return [a method2: 0];
}
