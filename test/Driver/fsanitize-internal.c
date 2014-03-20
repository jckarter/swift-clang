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

//
// We do support the undefined behavior sanitizer options on the internal branches.
//

// RUN: %clang -target x86_64-linux-gnu -fsanitize=undefined-trap -fsanitize-undefined-trap-on-error %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-TRAP
// RUN: %clang -target x86_64-linux-gnu -fsanitize-undefined-trap-on-error -fsanitize=undefined-trap %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED-TRAP
// CHECK-UNDEFINED-TRAP: "-fsanitize={{((signed-integer-overflow|integer-divide-by-zero|float-divide-by-zero|shift|unreachable|return|vla-bound|alignment|null|object-size|float-cast-overflow|array-bounds|enum|bool),?){14}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=undefined %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNDEFINED
// CHECK-UNDEFINED: "-fsanitize={{((signed-integer-overflow|integer-divide-by-zero|float-divide-by-zero|shift|unreachable|return|vla-bound|alignment|null|vptr|object-size|float-cast-overflow|array-bounds|enum|bool|function),?){16}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=alignment %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-ALIGNMENT
// CHECK-ALIGNMENT-NOT: unsupported argument 'alignment' to option 'fsanitize='
// CHECK-ALIGNMENT: "-fsanitize={{((alignment),?){1}"}}


// RUN: %clang -target x86_64-linux-gnu -fsanitize=bounds %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-BOUNDS
// CHECK-BOUNDS-NOT: unsupported argument 'bounds' to option 'fsanitize='
// CHECK-BOUNDS: "-fsanitize={{((array-bounds|local-bounds),?){2}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=float-cast-overflow %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-FLOAT-CAST-OVERFLOW
// CHECK-FLOAT-CAST-OVERFLOW-NOT: unsupported argument 'float-cast-overflow' to option 'fsanitize='
// CHECK-FLOAT-CAST-OVERFLOW: "-fsanitize={{((float-cast-overflow),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=float-divide-by-zero %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-FLOAT-DIVIDE-BY-ZERO
// CHECK-FLOAT-DIVIDE-BY-ZERO-NOT: unsupported argument 'float-divide-by-zero' to option 'fsanitize='
// CHECK-FLOAT-DIVIDE-BY-ZERO: "-fsanitize={{((float-divide-by-zero),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=integer-divide-by-zero %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-INTEGER-DIVIDE-BY-ZERO
// CHECK-INTEGER-DIVIDE-BY-ZERO-NOT: unsupported argument 'integer-divide-by-zero' to option 'fsanitize='
// CHECK-INTEGER-DIVIDE-BY-ZERO: "-fsanitize={{((integer-divide-by-zero),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=null %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-NULL
// CHECK-NULL-NOT: unsupported argument 'null' to option 'fsanitize='
// CHECK-NULL: "-fsanitize={{((null),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=object-size %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-OBJECT-SIZE
// CHECK-OBJECT-SIZE-NOT: unsupported argument 'object-size' to option 'fsanitize='
// CHECK-OBJECT-SIZE: "-fsanitize={{((object-size),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=return %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-RETURN
// CHECK-RETURN-NOT: unsupported argument 'return' to option 'fsanitize='
// CHECK-RETURN: "-fsanitize={{((return),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=shift %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SHIFT
// CHECK-SHIFT-NOT: unsupported argument 'shift' to option 'fsanitize='
// CHECK-SHIFT: "-fsanitize={{((shift),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=signed-integer-overflow %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-SIGNED-INTEGER-OVERFLOW
// CHECK-SIGNED-INTEGER-OVERFLOW-NOT: unsupported argument 'signed-integer-overflow' to option 'fsanitize='
// CHECK-SIGNED-INTEGER-OVERFLOW: "-fsanitize={{((signed-integer-overflow),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=unreachable %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-UNREACHABLE
// CHECK-UNREACHABLE-NOT: unsupported argument 'unreachable' to option 'fsanitize='
// CHECK-UNREACHABLE: "-fsanitize={{((unreachable),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=vla-bound %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-VLA-BOUND
// CHECK-VLA-BOUND-NOT: unsupported argument 'vla-bound' to option 'fsanitize='
// CHECK-VLA-BOUND: "-fsanitize={{((vla-bound),?){1}"}}

// RUN: %clang -target x86_64-linux-gnu -fsanitize=vptr %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-VPTR
// CHECK-VPTR-NOT: unsupported argument 'vptr' to option 'fsanitize='
// CHECK-VPTR: "-fsanitize={{((vptr),?){1}"}}
