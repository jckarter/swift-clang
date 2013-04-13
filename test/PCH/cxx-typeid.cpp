// XFAIL: hexagon
// Test this without pch.
// RUN: %clang -include %S/cxx-typeid.h -fsyntax-only -Xclang -stdlib=libstdc++ -verify %s

// RUN: %clang -ccc-pch-is-pch -x c++-header -stdlib=libstdc++ -o %t.gch %S/cxx-typeid.h
// RUN: %clang -ccc-pch-is-pch -include %t -fsyntax-only -Xclang -stdlib=libstdc++ -verify %s 

// expected-no-diagnostics

void f() {
    (void)typeid(int);
}
