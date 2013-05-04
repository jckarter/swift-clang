// RUN: %clang -### --migrate-xct %s 2>&1 | FileCheck %s

// CHECK: "-migrate" {{.*}} "-migration-for-xct" {{.*}} "-o" "{{.*}}driver.remap"
