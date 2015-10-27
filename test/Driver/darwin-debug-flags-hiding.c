// RUN: env RC_DEBUG_OPTIONS=1 %clang -target i386-apple-darwin9 -DKEEP=1 -DRC_HIDE_SECRET="Hello" -D RC_SHOW_SECRET -g %s -emit-llvm -S -o - | FileCheck %s

// CHECK: !0 = distinct !DICompileUnit(
// CHECK-SAME:                flags:
// CHECK-SAME:                -D KEEP=1
// CHECK-SAME-NOT:            -D
