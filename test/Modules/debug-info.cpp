// RUN: rm -rf %t

// Modules:
// RUN: %clang_cc1 -x objective-c++ -g -dwarf-ext-refs -fmodules -DMODULES -fmodules-cache-path=%t %s -I %S/Inputs -I %t -emit-llvm -o %t-mod.ll
// RUN: llvm-dwarfdump --debug-dump=info %t/*/DebugCXX-*.pcm | FileCheck %s --check-prefix=CHECK-AST
// RUN: llvm-dwarfdump --debug-dump=apple_types %t/*/DebugCXX-*.pcm | FileCheck %s --check-prefix=CHECK-MODULE
// RUN: cat %t-mod.ll |  FileCheck %s --check-prefix=CHECK-EXTREF-MODULE

// PCH:
// RUN: %clang_cc1 -x c++ -emit-pch -I %S/Inputs -o %t.pcm %S/Inputs/DebugCXX.h
// RUN: llvm-dwarfdump --debug-dump=info %t.pcm | FileCheck %s --check-prefix=CHECK-AST
// RUN: llvm-dwarfdump --debug-dump=apple_types %t.pcm | FileCheck %s --check-prefix=CHECK-PCH
// RUN: %clang_cc1 -g -dwarf-ext-refs -include-pch %t.pcm %s -emit-llvm -o %t-pch.ll %s
// RUN: cat %t-pch.ll |  FileCheck %s --check-prefix=CHECK-EXTREF-PCH


#ifdef MODULES
@import DebugCXX;
#endif

using DebugCXX::Foo;

Foo foo;
DebugCXX::Enum e;
DebugCXX::Bar<long> bar;
int Foo::static_member = -1;
// CHECK-AST: DW_TAG_compile_unit
// CHECK-AST: DW_AT_name {{.*(DebugCXX|<stdin>)}}
// CHECK-AST: DW_TAG_structure_type
// CHECK-AST: DW_AT_name {{.*}}"Foo"
// CHECK-AST: DW_TAG_member
// CHECK-AST: DW_AT_name {{.*}}"i"
// CHECK-MODULE: .apple_types contents:
// CHECK-MODULE: Name: {{.*}}"c:@N@DebugCXX@E@Enum"
// CHECK-MODULE: Name: {{.*}}"c:@N@DebugCXX@S@Foo"
// CHECK-PCH: .apple_types contents:
// CHECK-PCH: Name: {{.*}}"_ZTSN8DebugCXX3FooE"
// CHECK-PCH: Name: {{.*}}"_ZTSN8DebugCXX4EnumE"

// CHECK-EXTREF-MODULE: ![[FOO:.*]] = !MDExternalTypeRef(tag: DW_TAG_structure_type, file: ![[PCM:[0-9]+]], identifier: "c:@N@DebugCXX@S@Foo")
// CHECK-EXTREF-MODULE: !MDExternalTypeRef(tag: DW_TAG_enumeration_type, file: ![[PCM]], identifier: "c:@N@DebugCXX@E@Enum")
// CHECK-EXTREF-MODULE: !MDDerivedType(tag: DW_TAG_member, name: "static_member", scope: ![[FOO]]
// CHECK-EXTREF-MODULE: !MDImportedEntity(tag: DW_TAG_imported_declaration, {{.*}}, entity: ![[FOO]], line: 21)

// CHECK-EXTREF-PCH: ![[FOO:.*]] = !MDExternalTypeRef(tag: DW_TAG_structure_type, file: ![[PCM:[0-9]+]], identifier: "_ZTSN8DebugCXX3FooE")
// CHECK-EXTREF-PCH: !MDExternalTypeRef(tag: DW_TAG_enumeration_type, file: ![[PCM]], identifier: "_ZTSN8DebugCXX4EnumE")
// CHECK-EXTREF-PCH: !MDDerivedType(tag: DW_TAG_member, name: "static_member", scope: ![[FOO]]
// CHECK-EXTREF-PCH: !MDImportedEntity(tag: DW_TAG_imported_declaration, {{.*}}, entity: ![[FOO]], line: 21)
