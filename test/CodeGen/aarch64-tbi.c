// REQUIRES: aarch64-registered-target

// RUN: %clang -target arm64-apple-ios8.0 -mcpu=cyclone \
// RUN:   -S -O3 -o - %s | \
// RUN:   FileCheck --check-prefix=BOTH --check-prefix=NO_TBI %s

// RUN: %clang -target arm64-apple-ios8.0 -mcpu=cyclone \
// RUN:   -S -O3 -o - %s -mllvm -aarch64-use-tbi | \
// RUN:   FileCheck --check-prefix=BOTH --check-prefix=TBI %s

char f (long long p)
// BOTH-LABEL: f:
{
  long long p1 = p & 72057594037927935;
// TBI-NOT: and
// NO_TBI: and x
  char *p2 = (char*)p1;
  return *p2;
}
