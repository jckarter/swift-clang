// RUN: rm -rf %t
// RUN: %clang_cc1 -x objective-c -Wauto-import -fmodule-cache-path %t -fmodules -F %S/Inputs %s -verify

@import Module.Sub;

void test_Module_Sub() {
  int *ip = Module_Sub;
}

@import Module.Buried.Treasure;

void dig() {
  unsigned *up = Buried_Treasure;
}

