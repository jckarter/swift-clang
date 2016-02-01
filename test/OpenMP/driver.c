// Test that by default -fnoopenmp-use-tls is passed to frontend.
//
// APPLE INTERNAL: We do not yet support OpenMP (rdar://problem/12844522)
// RUN: true
// RUN-DISABLED: %clang %s -### -o %t.o 2>&1 -fopenmp=libomp | FileCheck --check-prefix=CHECK-DEFAULT %s
// CHECK-DEFAULT: -cc1
// CHECK-DEFAULT-NOT: -fnoopenmp-use-tls
//
// RUN-DISABLED: %clang %s -### -o %t.o 2>&1 -fopenmp=libomp -fnoopenmp-use-tls | FileCheck --check-prefix=CHECK-NO-TLS %s
// CHECK-NO-TLS: -cc1
// CHECK-NO-TLS-SAME: -fnoopenmp-use-tls
//
