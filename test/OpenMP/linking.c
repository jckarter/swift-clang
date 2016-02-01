// Test the that the driver produces reasonable linker invocations with
// -fopenmp or -fopenmp|libgomp.
//
// APPLE INTERNAL: We do not yet support OpenMP (rdar://problem/12844522)
// RUN: true
//
// FIXME: Replace DEFAULT_OPENMP_LIB below with the value chosen at configure time.
//
// RUN-DISABLED: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN-DISABLED:     -fopenmp -target i386-unknown-linux \
// RUN-DISABLED:   | FileCheck --check-prefix=CHECK-LD-32 %s
// CHECK-LD-32: "{{.*}}ld{{(.exe)?}}"
// CHECK-LD-32: "-l[[DEFAULT_OPENMP_LIB:[^"]*]]" "-lgcc"
// CHECK-LD-32: "-lpthread" "-lc"
//
// RUN-DISABLED: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN-DISABLED:     -fopenmp -target x86_64-unknown-linux \
// RUN-DISABLED:   | FileCheck --check-prefix=CHECK-LD-64 %s
// CHECK-LD-64: "{{.*}}ld{{(.exe)?}}"
// CHECK-LD-64: "-l[[DEFAULT_OPENMP_LIB:[^"]*]]" "-lgcc"
// CHECK-LD-64: "-lpthread" "-lc"
//
// RUN-DISABLED: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN-DISABLED:     -fopenmp=libgomp -target i386-unknown-linux \
// RUN-DISABLED:   | FileCheck --check-prefix=CHECK-GOMP-LD-32 %s
// CHECK-GOMP-LD-32: "{{.*}}ld{{(.exe)?}}"
// CHECK-GOMP-LD-32: "-lgomp" "-lrt" "-lgcc"
// CHECK-GOMP-LD-32: "-lpthread" "-lc"
//
// RUN-DISABLED: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN-DISABLED:     -fopenmp=libgomp -target x86_64-unknown-linux \
// RUN-DISABLED:   | FileCheck --check-prefix=CHECK-GOMP-LD-64 %s
// CHECK-GOMP-LD-64: "{{.*}}ld{{(.exe)?}}"
// CHECK-GOMP-LD-64: "-lgomp" "-lrt" "-lgcc"
// CHECK-GOMP-LD-64: "-lpthread" "-lc"
//
// RUN-DISABLED: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN-DISABLED:     -fopenmp -target i386-unknown-linux \
// RUN-DISABLED:   | FileCheck --check-prefix=CHECK-IOMP5-LD-32 %s
// CHECK-IOMP5-LD-32: "{{.*}}ld{{(.exe)?}}"
// CHECK-IOMP5-LD-32: "-l[[DEFAULT_OPENMP_LIB:[^"]*]]" "-lgcc"
// CHECK-IOMP5-LD-32: "-lpthread" "-lc"
//
// RUN-DISABLED: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN-DISABLED:     -fopenmp -target x86_64-unknown-linux \
// RUN-DISABLED:   | FileCheck --check-prefix=CHECK-IOMP5-LD-64 %s
// CHECK-IOMP5-LD-64: "{{.*}}ld{{(.exe)?}}"
// CHECK-IOMP5-LD-64: "-l[[DEFAULT_OPENMP_LIB:[^"]*]]" "-lgcc"
// CHECK-IOMP5-LD-64: "-lpthread" "-lc"
//
// RUN-DISABLED: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN-DISABLED:     -fopenmp=lib -target i386-unknown-linux \
// RUN-DISABLED:   | FileCheck --check-prefix=CHECK-LIB-LD-32 %s
// CHECK-LIB-LD-32: error: unsupported argument 'lib' to option 'fopenmp='
//
// RUN-DISABLED: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN-DISABLED:     -fopenmp=lib -target x86_64-unknown-linux \
// RUN-DISABLED:   | FileCheck --check-prefix=CHECK-LIB-LD-64 %s
// CHECK-LIB-LD-64: error: unsupported argument 'lib' to option 'fopenmp='
//
// RUN-DISABLED: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN-DISABLED:     -fopenmp -fopenmp=libgomp -target i386-unknown-linux \
// RUN-DISABLED:   | FileCheck --check-prefix=CHECK-LD-OVERRIDE-32 %s
// CHECK-LD-OVERRIDE-32: "{{.*}}ld{{(.exe)?}}"
// CHECK-LD-OVERRIDE-32: "-lgomp" "-lrt" "-lgcc"
// CHECK-LD-OVERRIDE-32: "-lpthread" "-lc"
//
// RUN-DISABLED: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN-DISABLED:     -fopenmp -fopenmp=libgomp -target x86_64-unknown-linux \
// RUN-DISABLED:   | FileCheck --check-prefix=CHECK-LD-OVERRIDE-64 %s
// CHECK-LD-OVERRIDE-64: "{{.*}}ld{{(.exe)?}}"
// CHECK-LD-OVERRIDE-64: "-lgomp" "-lrt" "-lgcc"
// CHECK-LD-OVERRIDE-64: "-lpthread" "-lc"
//
// RUN-DISABLED: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN-DISABLED:     -fopenmp=libomp -target x86_64-msvc-win32 \
// RUN-DISABLED:   | FileCheck --check-prefix=CHECK-MSVC-LINK-64 %s
// CHECK-MSVC-LINK-64: link.exe
// CHECK-MSVC-LINK-64-SAME: -nodefaultlib:vcomp.lib
// CHECK-MSVC-LINK-64-SAME: -nodefaultlib:vcompd.lib
// CHECK-MSVC-LINK-64-SAME: -libpath:{{.+}}/../lib
// CHECK-MSVC-LINK-64-SAME: -defaultlib:libomp.lib
//
// RUN-DISABLED: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN-DISABLED:     -fopenmp=libiomp5 -target x86_64-msvc-win32 \
// RUN-DISABLED:   | FileCheck --check-prefix=CHECK-MSVC-ILINK-64 %s
// CHECK-MSVC-ILINK-64: link.exe
// CHECK-MSVC-ILINK-64-SAME: -nodefaultlib:vcomp.lib
// CHECK-MSVC-ILINK-64-SAME: -nodefaultlib:vcompd.lib
// CHECK-MSVC-ILINK-64-SAME: -libpath:{{.+}}/../lib
// CHECK-MSVC-ILINK-64-SAME: -defaultlib:libiomp5md.lib
//
