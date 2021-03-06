// RUN: cd %S
// RUN: %clang_cc1 -include Inputs/test3.h -E -H -o %t.out %s 2> %t.stderr
// RUN: FileCheck < %t.stderr %s

// CHECK-NOT: test3.h
// CHECK: . {{.*test.h}}
// CHECK: .. {{.*test2.h}}

// RUN: %clang_cc1 -include Inputs/test3.h -E --show-includes -o %t.out %s > %t.stdout
// RUN: FileCheck --check-prefix=MS < %t.stdout %s
// MS-NOT: test3.h
// MS: Note: including file: {{.*test.h}}
// MS: Note: including file:  {{.*test2.h}}
// MS-NOT: Note

// RUN: echo "fun:foo" > %t.blacklist
// RUN: %clang_cc1 -fsanitize=address -fdepfile-entry=%t.blacklist -E --show-includes -o %t.out %s > %t.stdout
// RUN: FileCheck --check-prefix=MS-BLACKLIST < %t.stdout %s
// MS-BLACKLIST: Note: including file: {{.*\.blacklist}}
// MS-BLACKLIST: Note: including file: {{.*test.h}}
// MS-BLACKLIST: Note: including file:  {{.*test2.h}}
// MS-BLACKLIST-NOT: Note

#include "Inputs/test.h"
