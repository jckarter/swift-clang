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
// CHECK-AST: DW_AT_name {{.*(DebugModule|<stdin>)}}
// CHECK-AST: DW_TAG_structure_type
// CHECK-AST: DW_AT_name {{.*}}"A"
// CHECK-AST: DW_TAG_subprogram
// CHECK-AST: DW_AT_name {{.*}}"-[A method2:]"

// CHECK-EXTREF: distinct !DICompositeType(tag: DW_TAG_structure_type, file: ![[PCM:[0-9]+]], flags: DIFlagExternalTypeRef, identifier: "c:objc(cs)A")
// CHECK-EXTREF: ![[PCM]] = !DIFile(filename: "{{.*}}.pcm", directory: "")
// CHECK-EXTREF: distinct !DICompositeType(tag: DW_TAG_enumeration_type, file: ![[PCM]], flags: DIFlagExternalTypeRef, identifier: "c:@EA@enum3")
// CHECK-EXTREF: !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !"c:objc(cs)A",
// CHECK-EXTREF: !DICompositeType(tag: DW_TAG_structure_type, name: "F"
// CHECK-EXTREF: !DIFile(filename: "{{.*}}DebugModule{{.*}}", directory: "{{.*}}test/Modules{{.*}}")
int foo(A *a, F *f) {
  enum3 e3 = ENUM_VAL3;
  return [a method2: ENUM_VALUE];
}
