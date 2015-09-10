// RUN: %clang -target x86_64-linux-gnu -fsanitize=undefined -fsanitize-trap=undefined %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-TRAP
// RUN: %clang -target x86_64-linux-gnu -fsanitize=undefined -fsanitize-trap=undefined -fno-sanitize-trap=signed-integer-overflow %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-TRAP2
// RUN: %clang -target x86_64-linux-gnu -fsanitize=undefined -fsanitize-undefined-trap-on-error %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-TRAP
// RUN: %clang -target x86_64-linux-gnu -fsanitize=undefined-trap -fsanitize-undefined-trap-on-error %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-TRAP
// RUN: %clang -target x86_64-linux-gnu -fsanitize-undefined-trap-on-error -fsanitize=undefined-trap %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-TRAP
// CHECK-UNDEFINED-TRAP: "-fsanitize={{((signed-integer-overflow|integer-divide-by-zero|float-divide-by-zero|shift-base|shift-exponent|unreachable|return|vla-bound|alignment|null|object-size|float-cast-overflow|array-bounds|enum|bool|returns-nonnull-attribute|nonnull-attribute|function),?){18}"}}
// CHECK-UNDEFINED-TRAP: "-fsanitize-trap=alignment,array-bounds,bool,enum,float-cast-overflow,float-divide-by-zero,function,integer-divide-by-zero,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift-base,shift-exponent,signed-integer-overflow,unreachable,vla-bound"
// CHECK-UNDEFINED-TRAP2: "-fsanitize-trap=alignment,array-bounds,bool,enum,float-cast-overflow,float-divide-by-zero,function,integer-divide-by-zero,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift-base,shift-exponent,unreachable,vla-bound"

// RUN: %clang -target x86_64-linux-gnu -fsanitize=undefined %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED
// CHECK-UNDEFINED: "-fsanitize={{((signed-integer-overflow|integer-divide-by-zero|float-divide-by-zero|function|shift-base|shift-exponent|unreachable|return|vla-bound|alignment|null|vptr|object-size|float-cast-overflow|array-bounds|enum|bool|returns-nonnull-attribute|nonnull-attribute),?){19}"}}

// XFAIL-UNDEFINED: *
// We don't support the sanitizer on internal branches, so fail this test.

// RUN: %clang -target x86_64-apple-darwin10 -fsanitize=undefined %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-DARWIN
// CHECK-UNDEFINED-DARWIN: "-fsanitize={{((signed-integer-overflow|integer-divide-by-zero|float-divide-by-zero|shift-base|shift-exponent|unreachable|return|vla-bound|alignment|null|object-size|float-cast-overflow|array-bounds|enum|bool|returns-nonnull-attribute|nonnull-attribute),?){17}"}}

// RUN: %clang -target i386-unknown-openbsd -fsanitize=undefined %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-OPENBSD
// CHECK-UNDEFINED-OPENBSD: "-fsanitize={{((signed-integer-overflow|integer-divide-by-zero|float-divide-by-zero|shift-base|shift-exponent|unreachable|return|vla-bound|alignment|null|object-size|float-cast-overflow|array-bounds|enum|bool|returns-nonnull-attribute|nonnull-attribute),?){17}"}}

// RUN: %clang -target i386-pc-win32 -fsanitize=undefined %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-WIN --check-prefix=CHECK-UNDEFINED-WIN32
// RUN: %clang -target i386-pc-win32 -fsanitize=undefined -x c++ %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-WIN --check-prefix=CHECK-UNDEFINED-WIN32 --check-prefix=CHECK-UNDEFINED-WIN-CXX
// RUN: %clang -target x86_64-pc-win32 -fsanitize=undefined %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-WIN --check-prefix=CHECK-UNDEFINED-WIN64
// RUN: %clang -target x86_64-pc-win32 -fsanitize=undefined -x c++ %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-WIN --check-prefix=CHECK-UNDEFINED-WIN64 --check-prefix=CHECK-UNDEFINED-WIN-CXX
// CHECK-UNDEFINED-WIN: "-fsanitize={{((signed-integer-overflow|integer-divide-by-zero|float-divide-by-zero|shift-base|shift-exponent|unreachable|return|vla-bound|alignment|null|object-size|float-cast-overflow|array-bounds|enum|bool|returns-nonnull-attribute|nonnull-attribute),?){17}"}}
// CHECK-UNDEFINED-WIN32-SAME: "--dependent-lib={{[^"]*}}ubsan_standalone-i386.lib"
// CHECK-UNDEFINED-WIN64-SAME: "--dependent-lib={{[^"]*}}ubsan_standalone-x86_64.lib"
// CHECK-UNDEFINED-WIN-CXX-SAME: "--dependent-lib={{[^"]*}}ubsan_standalone_cxx{{[^"]*}}.lib"

// RUN: %clang -target x86_64-linux-gnu -fsanitize=integer %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-INTEGER
// CHECK-INTEGER: "-fsanitize={{((signed-integer-overflow|unsigned-integer-overflow|integer-divide-by-zero|shift-base|shift-exponent),?){5}"}}

// RUN: %clang -fsanitize=bounds -### -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=CHECK-BOUNDS
// CHECK-BOUNDS: "-fsanitize={{((array-bounds|local-bounds),?){2}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=all %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-FSANITIZE-ALL
// CHECK-FSANITIZE-ALL: error: unsupported argument 'all' to option 'fsanitize='

// RUN: %clang -target x86_64-linux-gnu -fsanitize=address,undefined -fno-sanitize=all -fsanitize=thread %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-FNO-SANITIZE-ALL
// CHECK-FNO-SANITIZE-ALL: "-fsanitize=thread"

// RUN: %clang -target x86_64-linux-gnu -fsanitize=thread,undefined -fno-sanitize=thread -fno-sanitize=float-cast-overflow,vptr,bool,enum %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-PARTIAL-UNDEFINED
// CHECK-PARTIAL-UNDEFINED: "-fsanitize={{((signed-integer-overflow|integer-divide-by-zero|float-divide-by-zero|function|shift-base|shift-exponent|unreachable|return|vla-bound|alignment|null|object-size|array-bounds|returns-nonnull-attribute|nonnull-attribute),?){15}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=shift -fno-sanitize=shift-base %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-FSANITIZE-SHIFT-PARTIAL
// CHECK-FSANITIZE-SHIFT-PARTIAL: "-fsanitize=shift-exponent"

// RUN: %clang -target x86_64-linux-gnu -fsanitize=vptr -fsanitize-trap=undefined %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-VPTR-TRAP-UNDEF
// RUN: %clang -target x86_64-linux-gnu -fsanitize=vptr -fsanitize-undefined-trap-on-error %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-VPTR-TRAP-UNDEF
// CHECK-VPTR-TRAP-UNDEF: error: invalid argument '-fsanitize=vptr' not allowed with '-fsanitize-trap=undefined'

// RUN: %clang -target x86_64-linux-gnu -fsanitize=vptr -fno-rtti %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-VPTR-NO-RTTI
// CHECK-VPTR-NO-RTTI: '-fsanitize=vptr' not allowed with '-fno-rtti'

// RUN: %clang -target x86_64-linux-gnu -fsanitize=undefined -fno-rtti %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-NO-RTTI
// CHECK-UNDEFINED-NO-RTTI-NOT: vptr

// RUN: %clang -target x86_64-linux-gnu -fsanitize=address,thread -fno-rtti %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANA-SANT
// CHECK-SANA-SANT: '-fsanitize=address' not allowed with '-fsanitize=thread'

// RUN: %clang -target x86_64-linux-gnu -fsanitize=address,memory -pie -fno-rtti %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANA-SANM
// CHECK-SANA-SANM: '-fsanitize=address' not allowed with '-fsanitize=memory'

// RUN: %clang -target x86_64-linux-gnu -fsanitize=thread,memory -pie -fno-rtti %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANT-SANM
// CHECK-SANT-SANM: '-fsanitize=thread' not allowed with '-fsanitize=memory'

// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory,thread -pie -fno-rtti %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANM-SANT
// CHECK-SANM-SANT: '-fsanitize=thread' not allowed with '-fsanitize=memory'

// RUN: %clang -target x86_64-linux-gnu -fsanitize=leak,thread -pie -fno-rtti %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANL-SANT
// CHECK-SANL-SANT: '-fsanitize=leak' not allowed with '-fsanitize=thread'

// RUN: %clang -target x86_64-linux-gnu -fsanitize=leak,memory -pie -fno-rtti %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANL-SANM
// CHECK-SANL-SANM: '-fsanitize=leak' not allowed with '-fsanitize=memory'

// RUN: %clang -target x86_64-linux-gnu -fsanitize=kernel-address,thread -fno-rtti %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANKA-SANT
// CHECK-SANKA-SANT: '-fsanitize=kernel-address' not allowed with '-fsanitize=thread'

// RUN: %clang -target x86_64-linux-gnu -fsanitize=kernel-address,memory -pie -fno-rtti %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANKA-SANM
// CHECK-SANKA-SANM: '-fsanitize=kernel-address' not allowed with '-fsanitize=memory'

// RUN: %clang -target x86_64-linux-gnu -fsanitize=kernel-address,address -fno-rtti %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANKA-SANA
// CHECK-SANKA-SANA: '-fsanitize=kernel-address' not allowed with '-fsanitize=address'

// RUN: %clang -target x86_64-linux-gnu -fsanitize=kernel-address,leak -pie -fno-rtti %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANKA-SANL
// CHECK-SANKA-SANL: '-fsanitize=kernel-address' not allowed with '-fsanitize=leak'

// RUN: %clang -target x86_64-linux-gnu -fsanitize-memory-track-origins -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ONLY-TRACK-ORIGINS
// CHECK-ONLY-TRACK-ORIGINS: warning: argument unused during compilation: '-fsanitize-memory-track-origins'

// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fno-sanitize=memory -fsanitize-memory-track-origins -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-TRACK-ORIGINS-DISABLED-MSAN
// CHECK-TRACK-ORIGINS-DISABLED-MSAN-NOT: warning: argument unused

// RUN: %clang -target x86_64-linux-gnu -fsanitize=address %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-EXTRA-TRACK-ORIGINS
// CHECK-NO-EXTRA-TRACK-ORIGINS-NOT: "-fsanitize-memory-track-origins"

// RUN: %clang -target x86_64-linux-gnu -fsanitize=address -fsanitize=alignment -fsanitize=vptr -fno-sanitize=vptr %s -### 2>&1
// OK

// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -pie %s -### 2>&1
// OK

// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fsanitize-memory-track-origins -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-TRACK-ORIGINS-2
// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fsanitize-memory-track-origins=1 -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-TRACK-ORIGINS-1
// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fsanitize-memory-track-origins=1 -fsanitize-memory-track-origins -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-TRACK-ORIGINS-2
// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fsanitize-memory-track-origins=2 -fsanitize-memory-track-origins -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-TRACK-ORIGINS-2
// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fno-sanitize-memory-track-origins -fsanitize-memory-track-origins -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-TRACK-ORIGINS-2
// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fsanitize-memory-track-origins=0 -fsanitize-memory-track-origins=1 -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-TRACK-ORIGINS-1
// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fsanitize-memory-track-origins=0 -fsanitize-memory-track-origins -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-TRACK-ORIGINS-2

// CHECK-TRACK-ORIGINS-1: -fsanitize-memory-track-origins=1

// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fno-sanitize-memory-track-origins -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-TRACK-ORIGINS
// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fsanitize-memory-track-origins=0 -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-TRACK-ORIGINS
// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fsanitize-memory-track-origins -fno-sanitize-memory-track-origins -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-TRACK-ORIGINS
// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fsanitize-memory-track-origins -fsanitize-memory-track-origins=0 -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-TRACK-ORIGINS
// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fsanitize-memory-track-origins=2 -fno-sanitize-memory-track-origins -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-TRACK-ORIGINS
// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fsanitize-memory-track-origins=2 -fsanitize-memory-track-origins=0 -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-TRACK-ORIGINS
// CHECK-NO-TRACK-ORIGINS-NOT: sanitize-memory-track-origins

// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fsanitize-memory-track-origins=2 -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-TRACK-ORIGINS-2
// CHECK-TRACK-ORIGINS-2: -fsanitize-memory-track-origins=2

// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fsanitize-memory-track-origins=3 -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-TRACK-ORIGINS-3
// CHECK-TRACK-ORIGINS-3: error: invalid value '3' in '-fsanitize-memory-track-origins=3'

// RUN: %clang -target x86_64-linux-gnu -fsanitize=address -fsanitize-coverage=0 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANITIZE-COVERAGE-0
// RUN: %clang -target x86_64-linux-gnu -fsanitize=address                       %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANITIZE-COVERAGE-0
// CHECK-SANITIZE-COVERAGE-0-NOT: fsanitize-coverage
// RUN: %clang -target x86_64-linux-gnu -fsanitize=address -fsanitize-coverage=1 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANITIZE-COVERAGE-1
// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fsanitize-coverage=1 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANITIZE-COVERAGE-1
// RUN: %clang -target x86_64-linux-gnu -fsanitize=leak -fsanitize-coverage=1 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANITIZE-COVERAGE-1
// RUN: %clang -target x86_64-linux-gnu -fsanitize=undefined -fsanitize-coverage=1 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANITIZE-COVERAGE-1
// RUN: %clang -target x86_64-linux-gnu -fsanitize=bool -fsanitize-coverage=1 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANITIZE-COVERAGE-1
// RUN: %clang -target x86_64-linux-gnu -fsanitize=dataflow -fsanitize-coverage=1 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANITIZE-COVERAGE-1

// Remove the -NOT for the next three lines: rdar://problem/20859716
// CHECK-SANITIZE-COVERAGE-1-NOT: fsanitize-coverage=1
// RUN: %clang -target x86_64-linux-gnu -fsanitize=address -fsanitize-coverage=4 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANITIZE-COVERAGE-4
// CHECK-SANITIZE-COVERAGE-4-NOT: fsanitize-coverage=4
// RUN: %clang -target x86_64-linux-gnu -fsanitize=address -fsanitize-coverage=5 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANITIZE-COVERAGE-5
// CHECK-SANITIZE-COVERAGE-5-NOT: error: invalid value '5' in '-fsanitize-coverage=5'
// RUN: %clang -target x86_64-linux-gnu -fsanitize=thread   -fsanitize-coverage=1 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANITIZE-COVERAGE-UNUSED-0
// RUN: %clang -target x86_64-linux-gnu                     -fsanitize-coverage=1 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANITIZE-COVERAGE-UNUSED-2
// CHECK-SANITIZE-COVERAGE-UNUSED-0: error: unsupported argument 'thread' to option 'fsanitize='
// CHECK-SANITIZE-COVERAGE-UNUSED-2: argument unused during compilation: '-fsanitize-coverage=1'

// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory -fsanitize-memory-use-after-dtor -pie %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-MSAN-USE-AFTER-DTOR
// CHECK-MSAN-USE-AFTER-DTOR: -cc1{{.*}}-fsanitize-memory-use-after-dtor

// RUN: %clang -target x86_64-linux-gnu -fsanitize=address -fsanitize-address-field-padding=0 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ASAN-FIELD-PADDING-0
// CHECK-ASAN-FIELD-PADDING-0-NOT: -fsanitize-address-field-padding
// RUN: %clang -target x86_64-linux-gnu -fsanitize=address -fsanitize-address-field-padding=1 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ASAN-FIELD-PADDING-1
// CHECK-ASAN-FIELD-PADDING-1: -fsanitize-address-field-padding=1
// RUN: %clang -target x86_64-linux-gnu -fsanitize=address -fsanitize-address-field-padding=2 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ASAN-FIELD-PADDING-2
// CHECK-ASAN-FIELD-PADDING-2: -fsanitize-address-field-padding=2
// RUN: %clang -target x86_64-linux-gnu -fsanitize=address -fsanitize-address-field-padding=3 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ASAN-FIELD-PADDING-3
// CHECK-ASAN-FIELD-PADDING-3: error: invalid value '3' in '-fsanitize-address-field-padding=3'
// RUN: %clang -target x86_64-linux-gnu -fsanitize-address-field-padding=2 %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ASAN-FIELD-PADDING-NO-ASAN
// CHECK-ASAN-FIELD-PADDING-NO-ASAN: warning: argument unused during compilation: '-fsanitize-address-field-padding=2'
// RUN: %clang -target x86_64-linux-gnu -fsanitize-address-field-padding=2 -fsanitize=address -fno-sanitize=address %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ASAN-FIELD-PADDING-DISABLED-ASAN
// CHECK-ASAN-FIELD-PADDING-DISABLED-ASAN-NOT: warning: argument unused


// RUN: %clang -target x86_64-linux-gnu -fsanitize=vptr -fno-sanitize=vptr -fsanitize=undefined,address %s -### 2>&1
// OK

// RUN: %clang -target x86_64-linux-gnu -fsanitize=thread %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-TSAN-NO-PIE
// CHECK-TSAN-NO-PIE: "-mrelocation-model" "static"

// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-MSAN-NO-PIE
// CHECK-MSAN-NO-PIE: "-mrelocation-model" "pic" "-pic-level" "2" "-pie-level" "2"
// CHECK-MSAN-NO-PIE: "-pie"

// RUN: %clang -target arm-linux-androideabi -fsanitize=address %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ANDROID-ASAN-NO-PIE
// CHECK-ANDROID-ASAN-NO-PIE: "-mrelocation-model" "pic" "-pic-level" "2" "-pie-level" "2"
// CHECK-ANDROID-ASAN-NO-PIE: "-pie"

// RUN: %clang -target arm-linux-androideabi %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ANDROID-NO-ASAN
// CHECK-ANDROID-NO-ASAN: "-mrelocation-model" "pic"

// RUN: %clang -target x86_64-linux-gnu %s -fsanitize=undefined -### 2>&1 | FileCheck %s --check-prefix=CHECK-RECOVER
// RUN: %clang -target x86_64-linux-gnu %s -fsanitize=undefined -fsanitize-recover -### 2>&1 | FileCheck %s --check-prefix=CHECK-RECOVER
// RUN: %clang -target x86_64-linux-gnu %s -fsanitize=undefined -fsanitize-recover=all -### 2>&1 | FileCheck %s --check-prefix=CHECK-RECOVER-UNSUPPORTED
// RUN: %clang -target x86_64-linux-gnu %s -fsanitize=undefined -fno-sanitize-recover=undefined -### 2>&1 | FileCheck %s --check-prefix=CHECK-RECOVER-UNSUPPORTED
// RUN: %clang -target x86_64-linux-gnu %s -fsanitize=undefined -fno-sanitize-recover=all -fsanitize-recover=thread -### 2>&1 | FileCheck %s --check-prefix=CHECK-RECOVER-UNSUPPORTED
// RUN: %clang -target x86_64-linux-gnu %s -fsanitize=undefined -fno-sanitize-recover -fsanitize-recover=undefined -### 2>&1 | FileCheck %s --check-prefix=CHECK-RECOVER
// RUN: %clang -target x86_64-linux-gnu %s -fsanitize=undefined -fsanitize-recover=all -fno-sanitize-recover=undefined -### 2>&1 | FileCheck %s --check-prefix=CHECK-RECOVER-UNSUPPORTED
// RUN: %clang -target x86_64-linux-gnu %s -fsanitize=undefined -fno-sanitize-recover=all -fsanitize-recover=object-size,shift-base -### 2>&1 | FileCheck %s --check-prefix=CHECK-PARTIAL-RECOVER

// CHECK-RECOVER: "-fsanitize-recover={{((signed-integer-overflow|integer-divide-by-zero|float-divide-by-zero|function|shift-base|shift-exponent|vla-bound|alignment|null|vptr|object-size|float-cast-overflow|array-bounds|enum|bool|returns-nonnull-attribute|nonnull-attribute),?){17}"}}
// CHECK-NO-RECOVER-NOT: sanitize-recover
// CHECK-PARTIAL-RECOVER: "-fsanitize-recover={{((object-size|shift-base),?){2}"}}
// CHECK-RECOVER-UNSUPPORTED: error: unsupported argument 

// RUN: %clang -target x86_64-linux-gnu %s -fsanitize=undefined -fsanitize-recover=address,foobar,object-size,unreachable -### 2>&1 | FileCheck %s --check-prefix=CHECK-DIAG-RECOVER
// CHECK-DIAG-RECOVER: unsupported argument 'foobar' to option 'fsanitize-recover='
// CHECK-DIAG-RECOVER: unsupported argument 'address,unreachable' to option 'fsanitize-recover='

// RUN: %clang -target x86_64-linux-gnu %s -fsanitize=undefined -fsanitize-recover -fno-sanitize-recover -### 2>&1 | FileCheck %s --check-prefix=CHECK-DEPRECATED-RECOVER
// CHECK-DEPRECATED-RECOVER: argument '-fsanitize-recover' is deprecated, use '-fsanitize-recover=undefined,integer' instead
// CHECK-DEPRECATED-RECOVER: argument '-fno-sanitize-recover' is deprecated, use '-fno-sanitize-recover=undefined,integer' instead
// CHECK-DEPRECATED-RECOVER-NOT: is deprecated

// RUN: %clang -target x86_64-linux-gnu -fsanitize=leak %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANL
// CHECK-SANL: "-fsanitize=leak"

// RUN: %clang -target x86_64-linux-gnu -fsanitize=address,leak -fno-sanitize=address %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SANA-SANL-NO-SANA
// CHECK-SANA-SANL-NO-SANA: "-fsanitize=leak"

// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-MSAN
// CHECK-MSAN: "-fno-assume-sane-operator-new"
// RUN: %clang -target x86_64-linux-gnu -fsanitize=address %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ASAN
// CHECK-ASAN: "-fno-assume-sane-operator-new"

// RUN: %clang -target x86_64-linux-gnu -fsanitize=zzz %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-DIAG1
// CHECK-DIAG1: unsupported argument 'zzz' to option 'fsanitize='
// CHECK-DIAG1-NOT: unsupported argument 'zzz' to option 'fsanitize='

// RUN: %clang -target i686-linux-gnu -fsanitize=memory %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-MSAN-X86
// CHECK-MSAN-X86: error: unsupported option '-fsanitize=memory' for target 'i686--linux-gnu'

// RUN: %clang -target x86_64-apple-darwin10 -fsanitize=memory %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-MSAN-DARWIN
// CHECK-MSAN-DARWIN: unsupported option '-fsanitize=memory' for target 'x86_64-apple-darwin10'

// RUN: %clang -target x86_64-apple-darwin10 -fsanitize=memory -fno-sanitize=memory %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-MSAN-NOMSAN-DARWIN
// CHECK-MSAN-NOMSAN-DARWIN-NOT: unsupported option

// RUN: %clang -target x86_64-apple-darwin10 -fsanitize=memory -fsanitize=thread,memory %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-MSAN-TSAN-MSAN-DARWIN
// CHECK-MSAN-TSAN-MSAN-DARWIN: unsupported option '-fsanitize=thread,memory' for target 'x86_64-apple-darwin10'
// CHECK-MSAN-TSAN-MSAN-DARWIN-NOT: unsupported option

// RUN: %clang -target x86_64-apple-darwin10 -fsanitize=thread,memory -fsanitize=memory %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-TSAN-MSAN-MSAN-DARWIN
// CHECK-TSAN-MSAN-MSAN-DARWIN: unsupported option '-fsanitize=memory' for target 'x86_64-apple-darwin10'
// CHECK-TSAN-MSAN-MSAN-DARWIN: unsupported option '-fsanitize=thread' for target 'x86_64-apple-darwin10'
// CHECK-TSAN-MSAN-MSAN-DARWIN-NOT: unsupported option

// RUN: %clang -target x86_64-apple-darwin10 -fsanitize=function %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-FSAN-DARWIN
// CHECK-FSAN-DARWIN: unsupported option '-fsanitize=function' for target 'x86_64-apple-darwin10'

// RUN: %clang -target x86_64-apple-darwin10 -fsanitize=function -fsanitize=undefined %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-FSAN-UBSAN-DARWIN
// CHECK-FSAN-UBSAN-DARWIN: unsupported option '-fsanitize=function' for target 'x86_64-apple-darwin10'

// RUN: %clang -target x86_64-apple-darwin10 -mmacosx-version-min=10.8 -fsanitize=vptr %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-VPTR-DARWIN-OLD
// CHECK-VPTR-DARWIN-OLD: unsupported option '-fsanitize=vptr' for target 'x86_64-apple-darwin10'

// RUN: %clang -target x86_64-apple-darwin10 -mmacosx-version-min=10.9 -fsanitize=alignment,vptr %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-VPTR-DARWIN-NEW
// CHECK-VPTR-DARWIN-NEW: -fsanitize=alignment,vptr

// RUN: %clang -target armv7-apple-ios7 -miphoneos-version-min=7.0 -fsanitize=address %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ASAN-IOS
// CHECK-ASAN-IOS: -fsanitize=address

// RUN: %clang -target i386-pc-openbsd -fsanitize=address %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ASAN-OPENBSD
// CHECK-ASAN-OPENBSD: unsupported option '-fsanitize=address' for target 'i386-pc-openbsd'

// RUN: %clang -target x86_64-linux-gnu -fsanitize=cfi -flto -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CFI
// RUN: %clang -target x86_64-apple-darwin10 -fsanitize=cfi -flto -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CFI
// RUN: %clang -target x86_64-linux-gnu -fsanitize=cfi-derived-cast -flto -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CFI-DCAST
// RUN: %clang -target x86_64-linux-gnu -fsanitize=cfi-unrelated-cast -flto -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CFI-UCAST
// RUN: %clang -target x86_64-linux-gnu -flto -fsanitize=cfi-nvcall -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CFI-NVCALL
// RUN: %clang -target x86_64-linux-gnu -flto -fsanitize=cfi-vcall -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CFI-VCALL
// CHECK-CFI: -emit-llvm-bc{{.*}}-fsanitize=cfi-derived-cast,cfi-icall,cfi-unrelated-cast,cfi-nvcall,cfi-vcall
// CHECK-CFI-DCAST: -emit-llvm-bc{{.*}}-fsanitize=cfi-derived-cast
// CHECK-CFI-UCAST: -emit-llvm-bc{{.*}}-fsanitize=cfi-unrelated-cast
// CHECK-CFI-NVCALL: -emit-llvm-bc{{.*}}-fsanitize=cfi-nvcall
// CHECK-CFI-VCALL: -emit-llvm-bc{{.*}}-fsanitize=cfi-vcall

// RUN: %clang -target x86_64-linux-gnu -flto -fsanitize=cfi-derived-cast -fno-lto -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CFI-NOLTO
// CHECK-CFI-NOLTO: '-fsanitize=cfi-derived-cast' only allowed with '-flto'

// RUN: %clang -target mips-unknown-linux -fsanitize=cfi-icall %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CFI-ICALL-MIPS
// CHECK-CFI-ICALL-MIPS: unsupported option '-fsanitize=cfi-icall' for target 'mips-unknown-linux'

// RUN: %clang -target x86_64-linux-gnu -fsanitize-trap=address -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ASAN-TRAP
// CHECK-ASAN-TRAP: error: unsupported argument 'address' to option '-fsanitize-trap'

// RUN: %clang -target x86_64-apple-darwin10 -mmacosx-version-min=10.7 -flto -fsanitize=cfi-vcall -fno-sanitize-trap=cfi -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CFI-NOTRAP-OLD-MACOS
// CHECK-CFI-NOTRAP-OLD-MACOS: error: unsupported option '-fno-sanitize-trap=cfi-vcall' for target 'x86_64-apple-darwin10'

// RUN: %clang -target x86_64-pc-win32 -flto -fsanitize=cfi-vcall -fno-sanitize-trap=cfi -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CFI-NOTRAP-WIN
// CHECK-CFI-NOTRAP-WIN: -emit-llvm-bc
// CHECK-CFI-NOTRAP-WIN-NOT: -fsanitize-trap=cfi

// RUN: %clang_cl -fsanitize=address -c -MDd -### -- %s 2>&1 | FileCheck %s -check-prefix=CHECK-ASAN-DEBUGRTL
// RUN: %clang_cl -fsanitize=address -c -MTd -### -- %s 2>&1 | FileCheck %s -check-prefix=CHECK-ASAN-DEBUGRTL
// RUN: %clang_cl -fsanitize=address -c -LDd -### -- %s 2>&1 | FileCheck %s -check-prefix=CHECK-ASAN-DEBUGRTL
// RUN: %clang_cl -fsanitize=address -c -MD -MDd -### -- %s 2>&1 | FileCheck %s -check-prefix=CHECK-ASAN-DEBUGRTL
// RUN: %clang_cl -fsanitize=address -c -MT -MTd -### -- %s 2>&1 | FileCheck %s -check-prefix=CHECK-ASAN-DEBUGRTL
// RUN: %clang_cl -fsanitize=address -c -LD -LDd -### -- %s 2>&1 | FileCheck %s -check-prefix=CHECK-ASAN-DEBUGRTL
// CHECK-ASAN-DEBUGRTL: error: invalid argument
// CHECK-ASAN-DEBUGRTL: not allowed with '-fsanitize=address'
// CHECK-ASAN-DEBUGRTL: note: AddressSanitizer doesn't support linking with debug runtime libraries yet

// RUN: %clang_cl -fsanitize=address -c -MT -### -- %s 2>&1 | FileCheck %s -check-prefix=CHECK-ASAN-RELEASERTL
// RUN: %clang_cl -fsanitize=address -c -MD -### -- %s 2>&1 | FileCheck %s -check-prefix=CHECK-ASAN-RELEASERTL
// RUN: %clang_cl -fsanitize=address -c -LD -### -- %s 2>&1 | FileCheck %s -check-prefix=CHECK-ASAN-RELEASERTL
// RUN: %clang_cl -fsanitize=address -c -MTd -MT -### -- %s 2>&1 | FileCheck %s -check-prefix=CHECK-ASAN-RELEASERTL
// RUN: %clang_cl -fsanitize=address -c -MDd -MD -### -- %s 2>&1 | FileCheck %s -check-prefix=CHECK-ASAN-RELEASERTL
// RUN: %clang_cl -fsanitize=address -c -LDd -LD -### -- %s 2>&1 | FileCheck %s -check-prefix=CHECK-ASAN-RELEASERTL
// CHECK-ASAN-RELEASERTL-NOT: error: invalid argument

// RUN: %clang -fno-sanitize=safe-stack -### %s 2>&1 | FileCheck %s -check-prefix=NOSP
// NOSP-NOT: "-fsanitize=safe-stack"

// RUN: %clang -target x86_64-linux-gnu -fsanitize=safe-stack -### %s 2>&1 | FileCheck %s -check-prefix=SP
// RUN: %clang -target x86_64-linux-gnu -fsanitize=address,safe-stack -### %s 2>&1 | FileCheck %s -check-prefix=SP-ASAN
// RUN: %clang -target x86_64-linux-gnu -fstack-protector -fsanitize=safe-stack -### %s 2>&1 | FileCheck %s -check-prefix=SP
// RUN: %clang -target x86_64-linux-gnu -fsanitize=safe-stack -fstack-protector-all -### %s 2>&1 | FileCheck %s -check-prefix=SP
// SP-NOT: stack-protector
// SP: "-fsanitize=safe-stack"
// SP-ASAN-NOT: stack-protector
// SP-ASAN: "-fsanitize=address,safe-stack"

// RUN: %clang -target powerpc64-unknown-linux-gnu -fsanitize=memory %s -### 2>&1 | FileCheck %s -check-prefix=CHECK-SANM
// RUN: %clang -target powerpc64le-unknown-linux-gnu -fsanitize=memory %s -### 2>&1 | FileCheck %s -check-prefix=CHECK-SANM
// CHECK-SANM: "-fsanitize=memory"
