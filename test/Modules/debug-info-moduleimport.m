// RUN: rm -rf %t
// RUN: %clang_cc1 -g -dwarf-ext-refs -fmodules -DMODULES -fimplicit-module-maps -fmodules-cache-path=%t %s -I %S/Inputs -isysroot /tmp/.. -I %t -emit-llvm -o - | FileCheck %s
  
// CHECK: ![[CU:.*]] = distinct !DICompileUnit
@import DebugModule;
// CHECK: !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: ![[CU]], entity: ![[MODULE:.*]], line: 5)
// CHECK: ![[MODULE]] = !DIModule(scope: null, name: "DebugModule", configMacros: "-DMODULES=0", includePath: "{{.*}}/test/Modules/Inputs", isysroot: "/tmp/..")
