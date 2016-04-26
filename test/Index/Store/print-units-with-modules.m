// RUN: rm -rf %t.idx %t.mcp
// RUN: %clang -arch x86_64 -mmacosx-version-min=10.7 -c %s -o %t.o -index-store-path %t.idx -fmodules -fmodules-cache-path=%t.mcp -Xclang -fdisable-module-hash -I %S/Inputs/module
// RUN: c-index-test core -print-unit %t.idx | FileCheck %s

// XFAIL: linux

@import ModDep;
@import ModSystem;

// CHECK: ModDep.pcm
// CHECK: provider: clang-
// CHECK: is-system: 0
// CHECK: has-main: 0
// CHECK: main-path: {{$}}
// CHECK: out-file: {{.*}}/ModDep.pcm
// CHECK: DEPEND START
// CHECK: Unit | user | {{.*}}/ModTop.pcm | ModTop.pcm
// CHECK: Record | user | {{.*}}/Inputs/module/ModDep.h | ModDep.h
// CHECK: DEPEND END (2)

// CHECK: ModSystem.pcm
// CHECK: is-system: 1
// CHECK: has-main: 0
// CHECK: main-path: {{$}}
// CHECK: out-file: {{.*}}/ModSystem.pcm
// CHECK: DEPEND START
// CHECK: Record | system | {{.*}}/Inputs/module/ModSystem.h | ModSystem.h
// CHECK: DEPEND END (1)

// CHECK: ModTop.pcm
// CHECK: is-system: 0
// CHECK: has-main: 0
// CHECK: main-path: {{$}}
// CHECK: out-file: {{.*}}/ModTop.pcm
// CHECK: DEPEND START
// CHECK: Record | user | {{.*}}/Inputs/module/ModTop.h | ModTop.h
// CHECK: DEPEND END (1)

// CHECK: print-units-with-modules.m.tmp.o
// CHECK: is-system: 0
// CHECK: has-main: 1
// CHECK: main-path: {{.*}}/print-units-with-modules.m
// CHECK: out-file: {{.*}}/print-units-with-modules.m.tmp.o
// CHECK: DEPEND START
// CHECK: Unit | user | {{.*}}/ModDep.pcm | ModDep.pcm
// CHECK: Unit | system | {{.*}}/ModSystem.pcm | ModSystem.pcm
// CHECK: File | user | {{.*}}/print-units-with-modules.m | {{$}}
// CHECK: File | user | {{.*}}/Inputs/module/module.modulemap | {{$}}
// CHECK: DEPEND END (4)
