// Check that the toolchain include directory gets added to the default search
// paths.
//
// <rdar://problem/13135439> Include toolchain include directory in default
// header paths (for FlexLexer.h)

// Make a dummy toolchain resource layout.
//
// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir/toolchain/usr/bin
// RUN: mkdir -p %t.dir/toolchain/usr/include
// RUN: mkdir -p %t.dir/toolchain/usr/lib/clang/someversion/
//
// RUN: echo "#define A OK" > %t.dir/toolchain/usr/include/FlexLexer.h
// RUN: %clang_cc1 -fsyntax-only -v -verify -resource-dir %t.dir/toolchain/usr/lib/clang/someversion %s

// expected-no-diagnostics

#define OK 1
#include <FlexLexer.h>
#if A != OK
#error "expected dummy FlexLexer.h to define A OK"
#endif
