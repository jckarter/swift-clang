// APPLE INTERNAL: We do not yet support OpenMP (rdar://problem/12844522)
// RUN: not %clang -target x86_64-apple-macosx -fopenmp -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=CHECK-APPLE-DISABLED
//
// CHECK-APPLE-DISABLED: error: unsupported option
//
// RUN-DISABLED: %clang -target x86_64-linux-gnu -fopenmp=libgomp -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CC1-NO-OPENMP
// RUN-DISABLED: %clang -target x86_64-linux-gnu -fopenmp=libomp -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CC1-OPENMP
// RUN-DISABLED: %clang -target x86_64-linux-gnu -fopenmp=libgomp -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CC1-NO-OPENMP
// RUN-DISABLED: %clang -target x86_64-linux-gnu -fopenmp=libiomp5 -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CC1-OPENMP
// RUN-DISABLED: %clang -target x86_64-apple-darwin -fopenmp=libomp -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CC1-OPENMP
// RUN-DISABLED: %clang -target x86_64-apple-darwin -fopenmp=libgomp -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CC1-NO-OPENMP
// RUN-DISABLED: %clang -target x86_64-apple-darwin -fopenmp=libiomp5 -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CC1-OPENMP
// RUN-DISABLED: %clang -target x86_64-freebsd -fopenmp=libomp -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CC1-OPENMP
// RUN-DISABLED: %clang -target x86_64-freebsd -fopenmp=libgomp -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CC1-NO-OPENMP
// RUN-DISABLED: %clang -target x86_64-freebsd -fopenmp=libiomp5 -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CC1-OPENMP
// RUN-DISABLED: %clang -target x86_64-netbsd -fopenmp=libomp -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CC1-OPENMP
// RUN-DISABLED: %clang -target x86_64-netbsd -fopenmp=libgomp -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CC1-NO-OPENMP
// RUN-DISABLED: %clang -target x86_64-netbsd -fopenmp=libiomp5 -c %s -### 2>&1 | FileCheck %s --check-prefix=CHECK-CC1-OPENMP
//
// CHECK-CC1-OPENMP: "-cc1"
// CHECK-CC1-OPENMP: "-fopenmp"
//
// CHECK-CC1-NO-OPENMP: "-cc1"
// CHECK-CC1-NO-OPENMP-NOT: "-fopenmp"
//
// RUN-DISABLED: %clang -target x86_64-linux-gnu -fopenmp=libomp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-LD-OMP
// RUN-DISABLED: %clang -target x86_64-linux-gnu -fopenmp=libgomp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-LD-GOMP
// RUN-DISABLED: %clang -target x86_64-linux-gnu -fopenmp=libiomp5 %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-LD-IOMP5
//
// RUN-DISABLED: %clang -nostdlib -target x86_64-linux-gnu -fopenmp=libomp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-OMP
// RUN-DISABLED: %clang -nostdlib -target x86_64-linux-gnu -fopenmp=libgomp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-GOMP
// RUN-DISABLED: %clang -nostdlib -target x86_64-linux-gnu -fopenmp=libiomp5 %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-IOMP5
//
// RUN-DISABLED: %clang -target x86_64-darwin -fopenmp=libomp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-LD-OMP
// RUN-DISABLED: %clang -target x86_64-darwin -fopenmp=libgomp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-LD-GOMP
// RUN-DISABLED: %clang -target x86_64-darwin -fopenmp=libiomp5 %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-LD-IOMP5
//
// RUN-DISABLED: %clang -nostdlib -target x86_64-darwin -fopenmp=libomp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-OMP
// RUN-DISABLED: %clang -nostdlib -target x86_64-darwin -fopenmp=libgomp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-GOMP
// RUN-DISABLED: %clang -nostdlib -target x86_64-darwin -fopenmp=libiomp5 %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-IOMP5
//
// RUN-DISABLED: %clang -target x86_64-netbsd -fopenmp=libomp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-LD-OMP
// RUN-DISABLED: %clang -target x86_64-netbsd -fopenmp=libgomp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-LD-GOMP
// RUN-DISABLED: %clang -target x86_64-netbsd -fopenmp=libiomp5 %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-LD-IOMP5
//
// RUN-DISABLED: %clang -nostdlib -target x86_64-freebsd -fopenmp=libomp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-OMP
// RUN-DISABLED: %clang -nostdlib -target x86_64-freebsd -fopenmp=libgomp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-GOMP
// RUN-DISABLED: %clang -nostdlib -target x86_64-freebsd -fopenmp=libiomp5 %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-IOMP5
//
// RUN-DISABLED: %clang -nostdlib -target x86_64-netbsd -fopenmp=libomp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-OMP
// RUN-DISABLED: %clang -nostdlib -target x86_64-netbsd -fopenmp=libgomp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-GOMP
// RUN-DISABLED: %clang -nostdlib -target x86_64-netbsd -fopenmp=libiomp5 %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-NO-IOMP5
//
// CHECK-LD-OMP: "{{.*}}ld{{(.exe)?}}"
// CHECK-LD-OMP: "-lomp"
//
// CHECK-LD-GOMP: "{{.*}}ld{{(.exe)?}}"
// CHECK-LD-GOMP: "-lgomp"
//
// CHECK-LD-IOMP5: "{{.*}}ld{{(.exe)?}}"
// CHECK-LD-IOMP5: "-liomp5"
//
// CHECK-NO-OMP: "{{.*}}ld{{(.exe)?}}"
// CHECK-NO-OMP-NOT: "-lomp"
//
// CHECK-NO-GOMP: "{{.*}}ld{{(.exe)?}}"
// CHECK-NO-GOMP-NOT: "-lgomp"
//
// CHECK-NO-IOMP5: "{{.*}}ld{{(.exe)?}}"
// CHECK-NO-IOMP5-NOT: "-liomp5"
//
// We'd like to check that the default is sane, but until we have the ability
// to *always* semantically analyze OpenMP without always generating runtime
// calls (in the event of an unsupported runtime), we don't have a good way to
// test the CC1 invocation. Instead, just ensure we do eventually link *some*
// OpenMP runtime.
//
// RUN-DISABLED: %clang -target x86_64-linux-gnu -fopenmp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-LD-ANY
// RUN-DISABLED: %clang -target x86_64-darwin -fopenmp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-LD-ANY
// RUN-DISABLED: %clang -target x86_64-freebsd -fopenmp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-LD-ANY
// RUN-DISABLED: %clang -target x86_64-netbsd -fopenmp %s -o %t -### 2>&1 | FileCheck %s --check-prefix=CHECK-LD-ANY
//
// CHECK-LD-ANY: "{{.*}}ld{{(.exe)?}}"
// CHECK-LD-ANY: "-l{{(omp|gomp|iomp5)}}"
