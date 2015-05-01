// Check that the toolchain include directory gets added to the default search
// paths.
//
// <rdar://problem/13135439> Include toolchain include directory in default
// header paths (for FlexLexer.h)

// Make a dummy toolchain resource layout.
//
// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir/foo.xctoolchain/usr/bin
// RUN: mkdir -p %t.dir/foo.xctoolchain/usr/include
// RUN: mkdir -p %t.dir/foo.xctoolchain/usr/lib/clang/someversion/
// RUN: mkdir -p %t.dir/XcodeDefault.xctoolchain/usr/bin
// RUN: mkdir -p %t.dir/XcodeDefault.xctoolchain/usr/include
//
// RUN: echo "#define A OK" > %t.dir/XcodeDefault.xctoolchain/usr/include/FlexLexer.h
// RUN: %clang -fsyntax-only -v -Xclang -verify -isysroot / -resource-dir %t.dir/foo.xctoolchain/usr/lib/clang/someversion %s 2> %t.err
// RUN: FileCheck --check-prefix=CHECK-XCODE-TOOLCHAIN < %t.err %s
//
// CHECK-XCODE-TOOLCHAIN: #include <...> search starts here:
// CHECK-XCODE-TOOLCHAIN: /usr/local/include
// CHECK-XCODE-TOOLCHAIN: /foo.xctoolchain/usr/include
// CHECK-XCODE-TOOLCHAIN: /XcodeDefault.xctoolchain/usr/include

// Make a dummy command line tools resource layout.
//
// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir/CommandLineTools/usr/bin
// RUN: mkdir -p %t.dir/CommandLineTools/usr/include
// RUN: mkdir -p %t.dir/CommandLineTools/usr/lib/clang/someversion/
//
// RUN: echo "#define A OK" > %t.dir/CommandLineTools/usr/include/FlexLexer.h
// RUN: %clang -fsyntax-only -v -Xclang -verify -isysroot / -resource-dir %t.dir/CommandLineTools/usr/lib/clang/someversion %s

// Verify that we *don't* use this directory if it would resolve to
// some other '/usr/include' (which might be outside the sysroot).
//
// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir/sysroot/usr/bin
// RUN: mkdir -p %t.dir/sysroot/usr/include
// RUN: mkdir -p %t.dir/sysroot/usr/lib/clang/someversion/
//
// RUN: echo "#define A OK" > %t.dir/sysroot/usr/include/FlexLexer.h
// RUN: %clang -fsyntax-only -v -Xclang -verify -isysroot %t.dir/sysroot -resource-dir /usr/lib/clang/someversion %s 2> %t.err
// RUN: FileCheck --check-prefix=CHECK-NO-BAD-INCLUDE < %t.err %s

// CHECK-NO-BAD-INCLUDE: #include "..." search starts here
// CHECK-NO-BAD-INCLUDE: #include <...> search starts here
// CHECK-NO-BAD-INCLUDE-NOT: {{^ /usr/include}}


// expected-no-diagnostics

#define OK 1
#include <FlexLexer.h>
#if A != OK
#error "expected dummy FlexLexer.h to define A OK"
#endif
