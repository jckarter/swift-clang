// RUN: rm -rf %t
// RUN: %clang_cc1 -x objective-c -g -fmodules -fmodules-cache-path=%t %s -I %S/Inputs -emit-llvm -DWANT_FOO -o - | FileCheck %s

@import config;
// CHECK: !DICompileUnit(language: DW_LANG_ObjC, {{.*}}config
