// RUN: %clang -Wnon-modular-include-in-framework-module -Wno-trigraphs -Wno-missing-field-initializers -Wno-missing-prototypes -Wreturn-type -Wunreachable-code -Wdeprecated-objc-isa-usage -Wobjc-root-class -Wno-missing-braces -Wparentheses -Wswitch -Wunused-function -Wno-unused-label -Wno-unused-parameter -Wunused-variable -Wunused-value -Wempty-body -Wconditional-uninitialized -Wno-unknown-pragmas -Wno-shadow -Wno-four-char-constants -Wno-conversion -Wconstant-conversion -Wint-conversion -Wbool-conversion -Wenum-conversion -Wshorten-64-to-32 -Wno-newline-eof -Wdeprecated-declarations -Wno-sign-conversion -Wpointer-sign -c %s -O0 -g 
// RUN: %clang_cc1 -triple thumbv7-apple-macosx10.11.0 -emit-obj -mrelax-all -disable-free -disable-llvm-verifier -main-file-name %s -mrelocation-model pic -pic-level 2 -mthread-model posix -mdisable-fp-elim -masm-verbose -target-cpu cortex-a8 -target-feature +soft-float-abi -target-abi apcs-gnu -mfloat-abi soft -target-linker-version 253.9 -debug-info-kind=standalone -dwarf-version=2 -dwarf-column-info -O0 -Wnon-modular-include-in-framework-module -Wno-trigraphs -Wno-missing-field-initializers -Wno-missing-prototypes -Wreturn-type -Wunreachable-code -Wdeprecated-objc-isa-usage -Wobjc-root-class -Wno-missing-braces -Wparentheses -Wswitch -Wunused-function -Wno-unused-label -Wno-unused-parameter -Wunused-variable -Wunused-value -Wempty-body -Wconditional-uninitialized -Wno-unknown-pragmas -Wno-shadow -Wno-four-char-constants -Wno-conversion -Wconstant-conversion -Wint-conversion -Wbool-conversion -Wenum-conversion -Wshorten-64-to-32 -Wno-newline-eof -Wdeprecated-declarations -Wno-sign-conversion -Wpointer-sign -ferror-limit 19 -fmessage-length 0 -stack-protector 1 -mstackrealign -fblocks -fobjc-runtime=macosx-10.11.0 -fencode-extended-block-signature -fsjlj-exceptions -fmax-type-align=16 -fdiagnostics-show-option -x c %s 
// RUN: %clang_cc1 -triple arm64-apple-macosx10.11.0 -Wdeprecated-objc-isa-usage -Werror=deprecated-objc-isa-usage -emit-obj -mrelax-all -disable-free -disable-llvm-verifier -main-file-name %s -mrelocation-model pic -pic-level 2 -mthread-model posix -mdisable-fp-elim -masm-verbose -target-cpu cyclone -target-feature +neon -target-feature +crc -target-feature +crypto -target-feature +zcm -target-feature +zcz -target-abi darwinpcs -target-linker-version 253.9 -debug-info-kind=standalone -dwarf-version=2 -dwarf-column-info -O0 -Wnon-modular-include-in-framework-module -Wno-trigraphs -Wno-missing-field-initializers -Wno-missing-prototypes -Wreturn-type -Wunreachable-code -Wdeprecated-objc-isa-usage -Wobjc-root-class -Wno-missing-braces -Wparentheses -Wswitch -Wunused-function -Wno-unused-label -Wno-unused-parameter -Wunused-variable -Wunused-value -Wempty-body -Wconditional-uninitialized -Wno-unknown-pragmas -Wno-shadow -Wno-four-char-constants -Wno-conversion -Wconstant-conversion -Wint-conversion -Wbool-conversion -Wenum-conversion -Wshorten-64-to-32 -Wno-newline-eof -Wdeprecated-declarations -Wno-sign-conversion -Wpointer-sign -ferror-limit 19 -fmessage-length 0 -stack-protector 1 -mstackrealign -fallow-half-arguments-and-returns -fblocks -fobjc-runtime=macosx-10.11.0 -fencode-extended-block-signature -fmax-type-align=16 -fdiagnostics-show-option -x c %s 
// RUN: %clang_cc1 -triple x86_64-apple-macosx10.11.0 -Wdeprecated-objc-isa-usage -Werror=deprecated-objc-isa-usage -emit-obj -disable-free -disable-llvm-verifier -main-file-name %s -mrelocation-model pic -pic-level 2 -mthread-model posix -mdisable-fp-elim -masm-verbose -munwind-tables -target-cpu core2 -target-linker-version 253.9 -dwarf-column-info -Os -Wnon-modular-include-in-framework-module -Wno-trigraphs -Wno-missing-field-initializers -Wno-missing-prototypes -Wreturn-type -Wunreachable-code -Wdeprecated-objc-isa-usage -Wobjc-root-class -Wno-missing-braces -Wparentheses -Wswitch -Wunused-function -Wno-unused-label -Wno-unused-parameter -Wunused-variable -Wunused-value -Wempty-body -Wconditional-uninitialized -Wno-unknown-pragmas -Wno-shadow -Wno-four-char-constants -Wno-conversion -Wconstant-conversion -Wint-conversion -Wbool-conversion -Wenum-conversion -Wshorten-64-to-32 -Wno-newline-eof -Wdeprecated-declarations -Wno-sign-conversion -Wpointer-sign -ferror-limit 19 -fmessage-length 0 -stack-protector 1 -mstackrealign -fblocks -fobjc-runtime=macosx-10.11.0 -fencode-extended-block-signature -fmax-type-align=16 -fdiagnostics-show-option -vectorize-loops -vectorize-slp -x c %s 
// RUN: %clang_cc1 -triple x86_64-apple-macosx10.11.0 -Wdeprecated-objc-isa-usage -Werror=deprecated-objc-isa-usage -emit-obj -disable-free -disable-llvm-verifier -main-file-name %s -mrelocation-model pic -pic-level 2 -mthread-model posix -mdisable-fp-elim -masm-verbose -munwind-tables -target-cpu core2 -target-linker-version 253.9 -debug-info-kind=standalone -dwarf-version=2 -dwarf-column-info -O3 -Wnon-modular-include-in-framework-module -Wno-trigraphs -Wno-missing-field-initializers -Wno-missing-prototypes -Wreturn-type -Wunreachable-code -Wdeprecated-objc-isa-usage -Wobjc-root-class -Wno-missing-braces -Wparentheses -Wswitch -Wunused-function -Wno-unused-label -Wno-unused-parameter -Wunused-variable -Wunused-value -Wempty-body -Wconditional-uninitialized -Wno-unknown-pragmas -Wno-shadow -Wno-four-char-constants -Wno-conversion -Wconstant-conversion -Wint-conversion -Wbool-conversion -Wenum-conversion -Wshorten-64-to-32 -Wno-newline-eof -Wdeprecated-declarations -Wno-sign-conversion -Wpointer-sign -ferror-limit 19 -fmessage-length 0 -stack-protector 1 -mstackrealign -fblocks -fobjc-runtime=macosx-10.11.0 -fencode-extended-block-signature -fmax-type-align=16 -fdiagnostics-show-option -vectorize-loops -vectorize-slp -x c %s 
// RUN: %clang_cc1 -triple i386-apple-macosx10.11.0 -emit-obj -mrelax-all -disable-free -disable-llvm-verifier -main-file-name %s -mrelocation-model pic -pic-level 2 -mthread-model posix -mdisable-fp-elim -masm-verbose -target-cpu yonah -target-linker-version 253.9 -debug-info-kind=standalone -dwarf-version=2 -dwarf-column-info -O0 -Wnon-modular-include-in-framework-module -Wno-trigraphs -Wno-missing-field-initializers -Wno-missing-prototypes -Wreturn-type -Wunreachable-code -Wdeprecated-objc-isa-usage -Wobjc-root-class -Wno-missing-braces -Wparentheses -Wswitch -Wunused-function -Wno-unused-label -Wno-unused-parameter -Wunused-variable -Wunused-value -Wempty-body -Wconditional-uninitialized -Wno-unknown-pragmas -Wno-shadow -Wno-four-char-constants -Wno-conversion -Wconstant-conversion -Wint-conversion -Wbool-conversion -Wenum-conversion -Wshorten-64-to-32 -Wno-newline-eof -Wdeprecated-declarations -Wno-sign-conversion -Wpointer-sign -ferror-limit 19 -fmessage-length 0 -stack-protector 1 -mstackrealign -fblocks -fobjc-runtime=macosx-fragile-10.11.0 -fobjc-subscripting-legacy-runtime -fencode-extended-block-signature -fmax-type-align=16 -fdiagnostics-show-option -x c %s 
void f0(void) {}
