; RUN: not %clang_cc1 -triple x86_64-apple-darwin10 -S %s -fno-gnu-inline-asm 2>&1 | \
; RUN: FileCheck %s
; CHECK: error: inline asm is used in the module with -fno-gnu-inline-asm

target triple = "x86_64-apple-darwin10"

module asm "INST"

define void @f0(i32 %arg) {
  call void asm "INST $0", "r" (i32 %arg)
  ret void
}
