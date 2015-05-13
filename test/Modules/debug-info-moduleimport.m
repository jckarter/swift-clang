// RUN: rm -rf %t
// RUN: %clang_cc1 -fmodules -x objective-c -fmodules-cache-path=%t -DWANT_FOO=1 -emit-module -fmodule-name=config %S/Inputs/module.map

// CHECK: ![[CU:.*]] = {{.*}}; [ DW_TAG_compile_unit ]
@import Module;
// CHECK: !{!"0x8\00[[@LINE-1]]\00", ![[CU]], ![[MODULE:.*]]} ; [ DW_TAG_imported_declaration ]
// CHECK: ![[MODULE]] = !{!"0x1e\00Module\00{{.*}}Inputs/Module.framework/Headers\00", ![[PCM:.*]], null} ; [ DW_TAG_module ] [Module]
// CHECK: ![[PCM]] = {{.*}}.pcm
