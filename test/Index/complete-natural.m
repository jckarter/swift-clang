// Note: the run lines follow their respective tests, since line/column
// matter in this test.

const char *in_string = "string";
char in_char = 'a';
// in comment
/* in comment */
#warning blarg
#error blarg
#pragma mark this is the spot
// RUN: c-index-test -code-completion-at=%s:4:32 %s > %t.1
// RUN: c-index-test service -code-completion-at=%s:4:32 %s > %t.2
// RUN: echo "DONE" >> %t.1
// RUN: echo "DONE" >> %t.2
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.1
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.2
// CHECK-CC1: Completion contexts:
// CHECK-CC1-NEXT: Natural language
// CHECK-CC1-NEXT: DONE
// RUN: c-index-test -code-completion-at=%s:5:18 %s > %t.1
// RUN: c-index-test service -code-completion-at=%s:5:18 %s > %t.2
// RUN: echo "DONE" >> %t.1
// RUN: echo "DONE" >> %t.2
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.1
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.2
// RUN: c-index-test -code-completion-at=%s:6:7 %s > %t.1
// RUN: c-index-test service -code-completion-at=%s:6:7 %s > %t.2
// RUN: echo "DONE" >> %t.1
// RUN: echo "DONE" >> %t.2
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.1
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.2
// RUN: c-index-test -code-completion-at=%s:7:7 %s > %t.1
// RUN: c-index-test service -code-completion-at=%s:7:7 %s > %t.2
// RUN: echo "DONE" >> %t.1
// RUN: echo "DONE" >> %t.2
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.1
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.2
// RUN: c-index-test -code-completion-at=%s:8:10 %s > %t.1
// RUN: c-index-test service -code-completion-at=%s:8:10 %s > %t.2
// RUN: echo "DONE" >> %t.1
// RUN: echo "DONE" >> %t.2
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.1
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.2
// RUN: c-index-test -code-completion-at=%s:9:9 %s > %t.1
// RUN: c-index-test service -code-completion-at=%s:9:9 %s > %t.2
// RUN: echo "DONE" >> %t.1
// RUN: echo "DONE" >> %t.2
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.1
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.2
// RUN: c-index-test -code-completion-at=%s:10:19 %s > %t.1
// RUN: c-index-test service -code-completion-at=%s:10:19 %s > %t.2
// RUN: echo "DONE" >> %t.1
// RUN: echo "DONE" >> %t.2
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.1
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.2

// Same tests as above, but with completion caching.
// RUN: env CINDEXTEST_EDITING=1 CINDEXTEST_COMPLETION_CACHING=1 c-index-test -code-completion-at=%s:4:32 %s > %t.1
// RUN: env CINDEXTEST_EDITING=1 CINDEXTEST_COMPLETION_CACHING=1 c-index-test service -code-completion-at=%s:4:32 %s > %t.2
// RUN: echo "DONE" >> %t.1
// RUN: echo "DONE" >> %t.2
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.1
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.2
// RUN: env CINDEXTEST_EDITING=1 CINDEXTEST_COMPLETION_CACHING=1 c-index-test -code-completion-at=%s:5:18 %s > %t.1
// RUN: env CINDEXTEST_EDITING=1 CINDEXTEST_COMPLETION_CACHING=1 c-index-test service -code-completion-at=%s:5:18 %s > %t.2
// RUN: echo "DONE" >> %t.1
// RUN: echo "DONE" >> %t.2
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.1
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.2
// RUN: env CINDEXTEST_EDITING=1 CINDEXTEST_COMPLETION_CACHING=1 c-index-test -code-completion-at=%s:6:7 %s > %t.1
// RUN: env CINDEXTEST_EDITING=1 CINDEXTEST_COMPLETION_CACHING=1 c-index-test service -code-completion-at=%s:6:7 %s > %t.2
// RUN: echo "DONE" >> %t.1
// RUN: echo "DONE" >> %t.2
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.1
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.2
// RUN: env CINDEXTEST_EDITING=1 CINDEXTEST_COMPLETION_CACHING=1 c-index-test -code-completion-at=%s:7:7 %s > %t.1
// RUN: env CINDEXTEST_EDITING=1 CINDEXTEST_COMPLETION_CACHING=1 c-index-test service -code-completion-at=%s:7:7 %s > %t.2
// RUN: echo "DONE" >> %t.1
// RUN: echo "DONE" >> %t.2
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.1
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.2
// RUN: env CINDEXTEST_EDITING=1 CINDEXTEST_COMPLETION_CACHING=1 c-index-test -code-completion-at=%s:8:10 %s > %t.1
// RUN: env CINDEXTEST_EDITING=1 CINDEXTEST_COMPLETION_CACHING=1 c-index-test service -code-completion-at=%s:8:10 %s > %t.2
// RUN: echo "DONE" >> %t.1
// RUN: echo "DONE" >> %t.2
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.1
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.2
// RUN: env CINDEXTEST_EDITING=1 CINDEXTEST_COMPLETION_CACHING=1 c-index-test -code-completion-at=%s:9:9 %s > %t.1
// RUN: env CINDEXTEST_EDITING=1 CINDEXTEST_COMPLETION_CACHING=1 c-index-test service -code-completion-at=%s:9:9 %s > %t.2
// RUN: echo "DONE" >> %t.1
// RUN: echo "DONE" >> %t.2
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.1
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.2
// RUN: env CINDEXTEST_EDITING=1 CINDEXTEST_COMPLETION_CACHING=1 c-index-test -code-completion-at=%s:10:19 %s > %t.1
// RUN: env CINDEXTEST_EDITING=1 CINDEXTEST_COMPLETION_CACHING=1 c-index-test service -code-completion-at=%s:10:19 %s > %t.2
// RUN: echo "DONE" >> %t.1
// RUN: echo "DONE" >> %t.2
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.1
// RUN: FileCheck -check-prefix=CHECK-CC1 %s < %t.2
