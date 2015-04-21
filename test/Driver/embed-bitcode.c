// RUN: %clang -ccc-print-bindings -c %s -fembed-bitcode 2>&1 | FileCheck %s
// CHECK: clang
// CHECK: clang

// RUN: %clang %s -fembed-bitcode 2>&1 -### | FileCheck %s -check-prefix=CHECK-CC
// CHECK-CC: -cc1
// CHECK-CC: -emit-llvm-bc
// CHECK-CC: -cc1
// CHECK-CC: -emit-obj
// CHECK-CC: -fembed-bitcode
// CHECK-CC: ld
// CHECK-CC: -bitcode_bundle

// RUN: %clang -c %s -flto -fembed-bitcode 2>&1 -### | FileCheck %s -check-prefix=CHECK-LTO
// CHECK-LTO: -cc1
// CHECK-LTO: -emit-llvm-bc
// CHECK-LTO-NOT: -cc1
// CHECK-LTO-NOT: -fembed-bitcode

// RUN: %clang -c %s -fembed-bitcode-marker 2>&1 -### | FileCheck %s -check-prefix=CHECK-MARKER
// CHECK-MARKER: -cc1
// CHECK-MARKER: -emit-obj
// CHECK-MARKER: -fembed-bitcode-marker
// CHECK-MARKER-NOT: -cc1

// RUN: %clang -c -x assembler %s -fembed-bitcode -### 2>&1 | FileCheck %s -check-prefix=CHECK-AS
// RUN: %clang -c -x assembler %s -fembed-bitcode-marker -### 2>&1 | FileCheck %s -check-prefix=CHECK-AS
// CHECK-AS: -cc1as
// CHECK-AS: -fembed-bitcode

// RUN: %clang -c %s -fembed-bitcode -mkernel -fapple-kext -ffunction-sections \
// RUN:   -fno-function-sections -fdata-sections -fno-data-sections \
// RUN:   -funique-section-names -fno-unique-section-names -mrestrict-it \
// RUN:   -mno-restrict-it -mstackrealgin -mno-stackrealign -mstack-alignment=8 \
// RUN:   -mcmodel=small -mlong-calls -mno-long-calls -ggnu-pubnames \
// RUN:   -gdwarf-arange -fdebug-types-section -fno-debug-types-section \
// RUN:   -fdwarf-directory-asm -fno-dwarf-directory-asm -ftrap-function=trap \
// RUN:   -ffixed-r9 -mfix-cortex-a53-835769 -mno-fix-cortex-a53-835769 \
// RUN:   -ffixed-x18 -mglobal-merge -mno-global-merge -mred-zone -mno-red-zone \
// RUN:   -Wa,-L -Xlinker,-L -mllvm -test -### 2>&1 | \
// RUN:   FileCheck %s -check-prefix=CHECK-UNSUPPORTED-OPT
// CHECK-UNSUPPORTED-OPT: -mkernel is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -fapple-kext is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -ffunction-sections is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -fno-function-sections is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -fdata-sections is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -fno-data-sections is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -funique-section-names is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -fno-unique-section-names is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -mrestrict-it is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -mno-restrict-it is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -mno-stackrealign is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -mstack-alignment= is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -mcmodel= is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -mlong-calls is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -mno-long-calls is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -ggnu-pubnames is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -fdebug-types-section is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -fno-debug-types-section is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -fdwarf-directory-asm is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -fno-dwarf-directory-asm is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -ftrap-function= is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -ffixed-r9 is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -mfix-cortex-a53-835769 is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -mno-fix-cortex-a53-835769 is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -ffixed-x18 is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -mglobal-merge is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -mno-global-merge is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -mred-zone is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -mno-red-zone is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -Wa, is not supported with -fembed-bitcode
// CHECK-UNSUPPORTED-OPT: -mllvm is not supported with -fembed-bitcode


// RUN: %clang -target armv7-apple-ios5.0.0 -S -fembed-bitcode %s -o - \
// RUN:   -O0 -fno-optimize-sibling-calls -flimited-precision=16 \
// RUN:   -fmath-errno -ffp-contract=fast -mabi=aapcs -ffast-math \
// RUN:   | FileCheck %s -check-prefix=CHECK-ARM-OPTIONS
// CHECK-ARM-OPTIONS: .section __LLVM,__cmdline
// CHECK-ARM-OPTIONS: -triple\000thumbv7-apple-ios5.0.0\000-S\000-disable-llvm-optzns\000-mdisable-tail-calls\000-mlimit-float-precision\00016\000-menable-no-infs\000-menable-no-nans\000-menable-unsafe-fp-math\000-fno-signed-zeros\000-freciprocal-math\000-ffp-contract=fast\000-target-abi\000aapcs\000-mfloat-abi\000soft\000-O0
