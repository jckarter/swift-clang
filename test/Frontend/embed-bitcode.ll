; check .ll input
; RUN: %clang_cc1 -triple thumbv7-apple-ios8.0.0 -emit-llvm \
; RUN:    -fembed-bitcode -x ir %s -o - \
; RUN:    | FileCheck %s
; RUN: %clang_cc1 -triple thumbv7-apple-ios8.0.0 -emit-llvm \
; RUN:    -fembed-bitcode-marker -x ir %s -o - \
; RUN:    | FileCheck %s -check-prefix=CHECK-MARKER

; check .bc input
; RUN: %clang_cc1 -triple thumbv7-apple-ios8.0.0 -emit-llvm-bc \
; RUN:    -x ir %s -o %t.bc
; RUN: %clang_cc1 -triple thumbv7-apple-ios8.0.0 -emit-llvm \
; RUN:    -fembed-bitcode -x ir %t.bc -o - \
; RUN:    | FileCheck %s
; RUN: %clang_cc1 -triple thumbv7-apple-ios8.0.0 -emit-llvm \
; RUN:    -fembed-bitcode-marker -x ir %t.bc -o - \
; RUN:    | FileCheck %s -check-prefix=CHECK-MARKER

; run through -fembed-bitcode twice and make sure it doesn't crash
; RUN: %clang_cc1 -triple thumbv7-apple-ios8.0.0 -emit-llvm-bc \
; RUN:    -fembed-bitcode -x ir %s -o - \
; RUN: | %clang_cc1 -triple thumbv7-apple-ios8.0.0 -emit-llvm \
; RUN:    -fembed-bitcode -x ir - -o /dev/null

; check the magic number of bitcode at the beginning of the string
; CHECK: @llvm.embedded.module
; CHECK: c"\DE\C0\17\0B
; CHECK: section "__LLVM,__bitcode"
; CHECK: @llvm.cmdline
; CHECK: section "__LLVM,__cmdline"

; CHECK-MARKER: @llvm.embedded.module
; CHECK-MARKER: constant [0 x i8] zeroinitializer
; CHECK-MARKER: section "__LLVM,__bitcode"
; CHECK-MARKER: @llvm.cmdline
; CHECK-MARKER: section "__LLVM,__cmdline"

target triple = "thumbv7-apple-ios8.0.0"

define i32 @f0() {
  ret i32 0
}
