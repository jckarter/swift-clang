//
// We do not support these sanitizers on the internal branches.
// rdar://12799898
//

// AddressSanitizer
// RUN: %clang -target x86_64-linux-gnu -fsanitize=address %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNSUPPORTED-ARG-ADDR
// CHECK-UNSUPPORTED-ARG-ADDR: unsupported argument 'address' to option 'fsanitize='

// RUN: %clang -target x86_64-linux-gnu -fsanitize=init-order %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNSUPPORTED-ARG-INIT-ORDER
// CHECK-UNSUPPORTED-ARG-INIT-ORDER: unsupported argument 'init-order' to option 'fsanitize='

// RUN: %clang -target x86_64-linux-gnu -fsanitize=use-after-return %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNSUPPORTED-ARG-USE-AFTER-RETURN
// CHECK-UNSUPPORTED-ARG-USE-AFTER-RETURN: unsupported argument 'use-after-return' to option 'fsanitize='

// RUN: %clang -target x86_64-linux-gnu -fsanitize=use-after-scope %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNSUPPORTED-ARG-USE-AFTER-SCOPE
// CHECK-UNSUPPORTED-ARG-USE-AFTER-SCOPE: unsupported argument 'use-after-scope' to option 'fsanitize='

// RUN: %clang -target x86_64-linux-gnu -fsanitize=address-full %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNSUPPORTED-ARG-ADDRESS-FULL
// CHECK-UNSUPPORTED-ARG-ADDRESS-FULL: unsupported argument 'address-full' to option 'fsanitize='

// MemorySanitizer
// RUN: %clang -target x86_64-linux-gnu -fsanitize=memory %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNSUPPORTED-ARG-MEMORY
// CHECK-UNSUPPORTED-ARG-MEMORY: unsupported argument 'memory' to option 'fsanitize='

// ThreadSanitizer
// RUN: %clang -target x86_64-linux-gnu -fsanitize=thread %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNSUPPORTED-ARG-THREAD
// CHECK-UNSUPPORTED-ARG-THREAD: unsupported argument 'thread' to option 'fsanitize='

// IntegerSanitizer
// RUN: %clang -target x86_64-linux-gnu -fsanitize=unsigned-integer-overflow %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNSUPPORTED-ARG-UIO
// CHECK-UNSUPPORTED-ARG-UIO: unsupported argument 'unsigned-integer-overflow' to option 'fsanitize='

// RUN: %clang -target x86_64-linux-gnu -fsanitize=integer %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNSUPPORTED-ARG-INTEGER
// CHECK-UNSUPPORTED-ARG-INTEGER: unsupported argument 'integer' to option 'fsanitize='

// RUN: %clang -target x86_64-linux-gnu -fsanitize=undefined %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNSUPPORTED-ARG-UNDEFINED
// CHECK-UNSUPPORTED-ARG-UNDEFINED:  unsupported argument 'undefined' to option 'fsanitize='

// RUN: %clang -target x86_64-linux-gnu -fsanitize=vptr %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNSUPPORTED-ARG-VPTR
// CHECK-UNSUPPORTED-ARG-VPTR: unsupported argument 'vptr' to option 'fsanitize='

//
// We do support the undefined behavior sanatizer options that don't require runtime support.
//

// RUN: %clang -target x86_64-linux-gnu -fsanitize=undefined-trap -fsanitize-undefined-trap-on-error %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-TRAP
// RUN: %clang -target x86_64-linux-gnu -fsanitize-undefined-trap-on-error -fsanitize=undefined-trap %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-TRAP
// CHECK-UNDEFINED-TRAP: "-fsanitize={{((alignment|array-bounds|bool|enum|float-cast-overflow|float-divide-by-zero|integer-divide-by-zero|null|object-size|return|returns-nonnull-attribute|shift|nonnull-attribute|signed-integer-overflow|unreachable|vla-bound),?){16}"}}
// CHECK-UNDEFINED-TRAP: "-fsanitize-undefined-trap-on-error"

// RUN: %clang -target x86_64-linux-gnu -fsanitize=undefined-trap %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-TRAP-REQUIRES-UNDEF-TRAP-ON-ERROR
// CHECK-UNDEFINED-TRAP-REQUIRES-UNDEF-TRAP-ON-ERROR: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=undefined-trap' option

// RUN: %clang -target x86_64-linux-gnu -fsanitize=alignment %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ALIGNMENT-REQUIRES-UNDEF-TRAP-ON-ERROR
// CHECK-ALIGNMENT-REQUIRES-UNDEF-TRAP-ON-ERROR: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=alignment' option
// RUN: %clang -target x86_64-linux-gnu -fsanitize=alignment -fsanitize-undefined-trap-on-error %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ALIGNMENT
// CHECK-ALIGNMENT-NOT: unsupported argument 'alignment' to option 'fsanitize='
// CHECK-ALIGNMENT-NOT: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=alignment' option
// CHECK-ALIGNMENT: "-fsanitize={{((alignment),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=float-cast-overflow %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-FLOAT-CAST-OVERFLOW-REQUIRES-UNDEF-TRAP-ON-ERROR
// CHECK-FLOAT-CAST-OVERFLOW-REQUIRES-UNDEF-TRAP-ON-ERROR: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=float-cast-overflow' option
// RUN: %clang -target x86_64-linux-gnu -fsanitize=float-cast-overflow -fsanitize-undefined-trap-on-error %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-FLOAT-CAST-OVERFLOW
// CHECK-FLOAT-CAST-OVERFLOW-NOT: unsupported argument 'float-cast-overflow' to option 'fsanitize='
// CHECK-FLOAT-CAST-OVERFLOW-NOT: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=float-cast-overflow' option
// CHECK-FLOAT-CAST-OVERFLOW: "-fsanitize={{((float-cast-overflow),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=null %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-NULL-REQUIRES-UNDEF-TRAP-ON-ERROR
// CHECK-NULL-REQUIRES-UNDEF-TRAP-ON-ERROR: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=null' option
// RUN: %clang -target x86_64-linux-gnu -fsanitize=null -fsanitize-undefined-trap-on-error %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-NULL
// CHECK-NULL-NOT: unsupported argument 'null' to option 'fsanitize='
// CHECK-NULL-REQUIRES-UNDEF-TRAP-ON-ERRORt: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=null' option
// CHECK-NULL: "-fsanitize={{((null),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=object-size %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-OBJECT-SIZE-REQUIRES-UNDEF-TRAP-ON-ERROR
// CHECK-OBJECT-SIZE-REQUIRES-UNDEF-TRAP-ON-ERROR: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=object-size' option
// RUN: %clang -target x86_64-linux-gnu -fsanitize=object-size -fsanitize-undefined-trap-on-error %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-OBJECT-SIZE
// CHECK-OBJECT-SIZE-NOT: unsupported argument 'object-size' to option 'fsanitize='
// CHECK-OBJECT-SIZE-NOT: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=object-size' option
// CHECK-OBJECT-SIZE: "-fsanitize={{((object-size),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=return %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-RETURN-REQUIRES-UNDEF-TRAP-ON-ERROR
// CHECK-RETURN-REQUIRES-UNDEF-TRAP-ON-ERROR: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=return' option
// RUN: %clang -target x86_64-linux-gnu -fsanitize=return -fsanitize-undefined-trap-on-error %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-RETURN
// CHECK-RETURN-NOT: unsupported argument 'return' to option 'fsanitize='
// CHECK-RETURN-NOT: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=return' option
// CHECK-RETURN: "-fsanitize={{((return),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=shift %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SHIFT-REQUIRES-UNDEF-TRAP-ON-ERROR
// CHECK-SHIFT-REQUIRES-UNDEF-TRAP-ON-ERROR: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=shift' option
// RUN: %clang -target x86_64-linux-gnu -fsanitize=shift -fsanitize-undefined-trap-on-error %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SHIFT
// CHECK-SHIFT-NOT: unsupported argument 'shift' to option 'fsanitize='
// CHECK-SHIFT-NOT: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=shift' option
// CHECK-SHIFT: "-fsanitize={{((shift),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=signed-integer-overflow %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SIGNED-INTEGER-OVERFLOW-REQUIRES-UNDEF-TRAP-ON-ERROR
// CHECK-SIGNED-INTEGER-OVERFLOW-REQUIRES-UNDEF-TRAP-ON-ERROR: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=signed-integer-overflow' option
// RUN: %clang -target x86_64-linux-gnu -fsanitize=signed-integer-overflow -fsanitize-undefined-trap-on-error %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SIGNED-INTEGER-OVERFLOW
// CHECK-SIGNED-INTEGER-OVERFLOW-NOT: unsupported argument 'signed-integer-overflow' to option 'fsanitize='
// CHECK-SIGNED-INTEGER-OVERFLOW-NOT: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=signed-integer-overflow' option
// CHECK-SIGNED-INTEGER-OVERFLOW: "-fsanitize={{((signed-integer-overflow),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=unreachable %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNREACHABLE-REQUIRES-UNDEF-TRAP-ON-ERROR
// CHECK-UNREACHABLE-REQUIRES-UNDEF-TRAP-ON-ERROR: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=unreachable' option
// RUN: %clang -target x86_64-linux-gnu -fsanitize=unreachable -fsanitize-undefined-trap-on-error %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNREACHABLE
// CHECK-UNREACHABLE-NOT: unsupported argument 'unreachable' to option 'fsanitize='
// CHECK-UNREACHABLE-NOT: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=unreachable' option
// CHECK-UNREACHABLE: "-fsanitize={{((unreachable),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=vla-bound %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-VLA-BOUND-REQUIRES-UNDEF-TRAP-ON-ERROR
// CHECK-VLA-BOUND-REQUIRES-UNDEF-TRAP-ON-ERROR: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=vla-bound' option
// RUN: %clang -target x86_64-linux-gnu -fsanitize=vla-bound -fsanitize-undefined-trap-on-error %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-VLA-BOUND
// CHECK-VLA-BOUND-NOT: unsupported argument 'vla-bound' to option 'fsanitize='
// CHECK-VLA-BOUND-NOT: '-fsanitize-undefined-trap-on-error' required with '-fsanitize=vla-bound' option
// CHECK-VLA-BOUND: "-fsanitize={{((vla-bound),?){1}"}}
