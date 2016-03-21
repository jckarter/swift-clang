// RUN: %clang %xcode_c_warnings -c %s -O0 -g 
// RUN: %clang_skip_driver -c %s -O0 -g -arch armv7 %xcode_c_warnings
// RUN: %clang_skip_driver -c %s -O0 -g -arch arm64 %xcode_c_warnings
// RUN: %clang_skip_driver -c %s -Os %xcode_c_warnings
// RUN: %clang_skip_driver -c %s -O3 -g %xcode_c_warnings
// RUN: %clang_skip_driver -c %s -O0 -g -arch i386 %xcode_c_warnings

void f0(void) {}
