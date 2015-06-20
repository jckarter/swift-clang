// RUN: rm -rf %t

// Modules:
// RUN: %clang_cc1 -x objective-c++ -std=c++11 -g -dwarf-ext-refs -fmodules -fimplicit-module-maps -DMODULES -fmodules-cache-path=%t %s -I %S/Inputs -I %t -emit-llvm -o %t-mod.ll
// RUN: llvm-dwarfdump --debug-dump=info %t/*/DebugCXX-*.pcm | FileCheck %s --check-prefix=CHECK-AST
// RUN: llvm-dwarfdump --debug-dump=apple_types %t/*/DebugCXX-*.pcm | FileCheck %s --check-prefix=CHECK-PCM
// RUN: cat %t-mod.ll |  FileCheck %s --check-prefix=CHECK-EXTREF-PCM

// PCH:
// RUN: %clang_cc1 -x c++ -std=c++11 -emit-pch -I %S/Inputs -o %t.pch %S/Inputs/DebugCXX.h
// RUN: llvm-dwarfdump --debug-dump=info %t.pch | FileCheck %s --check-prefix=CHECK-AST
// RUN: llvm-dwarfdump --debug-dump=apple_types %t.pch | FileCheck %s --check-prefix=CHECK-PCM
// RUN: %clang_cc1 -std=c++11 -g -dwarf-ext-refs -include-pch %t.pch %s -emit-llvm -o %t-pch.ll %s
// RUN: cat %t-pch.ll |  FileCheck %s --check-prefix=CHECK-EXTREF-PCM


#ifdef MODULES
@import DebugCXX;
#endif

using DebugCXX::Foo;

Foo foo;
DebugCXX::Enum e;
DebugCXX::Bar<long>  implicitBar;
DebugCXX::Bar<int>   explicitBar;
DebugCXX::FloatBar   typedefBar;
int Foo::static_member = -1;
enum {
  e3 = -1
} conflicting_uid = e3;
auto anon_enum = DebugCXX::e2;
char _anchor = anon_enum + conflicting_uid;
// CHECK-AST: DW_TAG_compile_unit
// CHECK-AST: DW_AT_name {{.*(DebugCXX|<stdin>)}}
// CHECK-AST: DW_TAG_structure_type
// CHECK-AST: DW_AT_name {{.*}}"Foo"
// CHECK-AST: DW_TAG_member
// CHECK-AST: DW_AT_name {{.*}}"value"
// CHECK-PCM: .apple_types contents:
// CHECK-PCM-DAG: Name: {{.*}}"_ZTSN8DebugCXX3FooE"
// CHECK-PCM-DAG: Name: {{.*}}"_ZTSN8DebugCXX4EnumE"

// CHECK-EXTREF-PCM: ![[ANON_ENUM:[0-9]+]] = !DICompositeType(tag: DW_TAG_enumeration_type
// CHECK-EXTREF-PCM-SAME: {{.*}}line: 13, {{.*}}, elements: !{{[0-9]+}})

// CHECK-EXTREF-PCM: !DICompositeType(tag: DW_TAG_structure_type, file: ![[PCM:[0-9]+]], flags: DIFlagExternalTypeRef, identifier: "_ZTSN8DebugCXX3FooE")
// CHECK-EXTREF-PCM: !DICompositeType(tag: DW_TAG_enumeration_type, file: ![[PCM]], flags: DIFlagExternalTypeRef, identifier:  "_ZTSN8DebugCXX4EnumE")
// CHECK-EXTREF-PCM: !DICompositeType({{.*}}identifier: "_ZTSN8DebugCXX3BarIiNS_6traitsIiEEEE")
// CHECK-EXTREF-PCM: !DIDerivedType(tag: DW_TAG_member, name: "static_member", scope: !"_ZTSN8DebugCXX3FooE"
// CHECK-EXTREF-PCM: !DIGlobalVariable(name: "anon_enum", {{.*}}, type: ![[ANON_ENUM]]
// CHECK-EXTREF-PCM: !DIImportedEntity(tag: DW_TAG_imported_declaration, {{.*}}, entity: !"_ZTSN8DebugCXX3FooE", line: 21)

