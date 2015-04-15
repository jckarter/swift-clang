// RUN: rm -rf %t

// Modules:
// RUN: %clang_cc1 -x objective-c++ -g -dwarf-ext-refs -fmodules -DMODULES -fmodules-cache-path=%t %s -I %S/Inputs -I %t -emit-llvm -o %t-mod.ll
// RUN: llvm-dwarfdump %t/*/DebugCXX-*.pcm | FileCheck %s --check-prefix=CHECK-AST
// RUN: cat %t-mod.ll |  FileCheck %s --check-prefix=CHECK-EXTREF

// PCH:
// RUN: %clang_cc1 -x c++ -emit-pch -I %S/Inputs -o %t.pcm %S/Inputs/DebugCXX.h
// RUN: llvm-dwarfdump %t.pcm | FileCheck %s --check-prefix=CHECK-AST
// RUN: %clang_cc1 -g -dwarf-ext-refs -include-pch %t.pcm %s -emit-llvm -o %t-pch.ll %s
// RUN: cat %t-pch.ll |  FileCheck %s --check-prefix=CHECK-EXTREF


#ifdef MODULES
@import DebugCXX;
#endif

using DebugCXX::Foo;

Foo foo;

// CHECK-AST: DW_TAG_compile_unit
// CHECK-AST: DW_AT_name {{.*(DebugCXX|<stdin>)}}
// CHECK-AST: DW_TAG_structure_type
// CHECK-AST: DW_AT_name {{.*}}"Foo"

// CHECK-EXTREF: ![[FOO:.*]] = !MDExternalTypeRef(tag: DW_TAG_structure_type, file: ![[PCM:[0-9]+]], identifier: "c:@N@DebugCXX@S@Foo")
// CHECK-EXTREF: !MDImportedEntity(tag: DW_TAG_imported_declaration, {{.*}}, entity: ![[FOO]], line: 19)
