// RUN: rm -rf %t
// RUN: %clang_cc1 -g -dwarf-ext-refs -fmodules -DMODULES -fmodules-cache-path=%t %s -I %S/Inputs -I %t -emit-llvm -o - | FileCheck %s
  
// CHECK: ![[CU:.*]] = distinct !DICompileUnit
@import DebugModule;
// CHECK: !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: ![[CU]], entity: ![[MODULE:.*]], line: 5)
// CHECK: ![[MODULE]] = !MDModule(name: "DebugModule", scope: null, file: ![[PCM:.*]])
// CHECK: ![[PCM]] = {{.*}}.pcm
