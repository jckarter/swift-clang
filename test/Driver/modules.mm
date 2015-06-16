// RUN: %clang -fmodules -### %s 2>&1 | FileCheck -check-prefix=CHECK-NO-MODULES %s
// RUN: %clang -fcxx-modules -### %s 2>&1 | FileCheck -check-prefix=CHECK-NO-MODULES %s
// CHECK-NO-MODULES-NOT: -fmodules

// RUN: %clang -fcxx-modules -fmodules -### %s 2>&1 | FileCheck -check-prefix=CHECK-HAS-MODULES %s
// CHECK-HAS-MODULES: -fmodules

// RUN: %clang -### %s 2>&1 | FileCheck -check-prefix=CHECK-NO-MAPS %s
// RUN: %clang -fimplicit-module-maps -### %s 2>&1 | FileCheck -check-prefix=CHECK-HAS-MAPS %s
// RUN: %clang -fmodules -fcxx-modules -### %s 2>&1 | FileCheck -check-prefix=CHECK-HAS-MAPS %s
// RUN: %clang -fmodules -fcxx-modules -fno-implicit-module-maps -### %s 2>&1 | FileCheck -check-prefix=CHECK-NO-MAPS %s
// CHECK-HAS-MAPS: -fimplicit-module-maps
// CHECK-NO-MAPS-NOT: -fimplicit-module-maps
