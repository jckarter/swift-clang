# This file sets up a CMakeCache for Apple-style stage2 bootstrap. It is
# specified by the stage1 build.

set(LLVM_TARGETS_TO_BUILD X86 ARM AArch64 CACHE STRING "") 
set(CLANG_VENDOR Apple CACHE STRING "")
set(LLVM_INCLUDE_EXAMPLES OFF CACHE BOOL "")
set(LLVM_INCLUDE_DOCS OFF CACHE BOOL "")
set(LLVM_TOOL_CLANG_TOOLS_EXTRA_BUILD OFF CACHE BOOL "")
set(CLANG_TOOL_LIBCLANG_BUILD OFF CACHE BOOL "")
set(CLANG_TOOL_C_ARCMT_TEST_BUILD OFF CACHE BOOL "")
set(CLANG_TOOL_C_INDEX_TEST_BUILD OFF CACHE BOOL "")
set(LLVM_BUILD_RUNTIME OFF CACHE BOOL "")
set(CLANG_LINKS_TO_CREATE clang++ cc c++ CACHE STRING "")
set(CMAKE_MACOSX_RPATH ON CACHE BOOL "")
set(LLVM_ENABLE_PIC OFF CACHE BOOL "")
set(LLVM_ENABLE_ZLIB OFF CACHE BOOL "")
set(LLVM_ENABLE_BACKTRACES OFF CACHE BOOL "")
set(CLANG_PLUGIN_SUPPORT OFF CACHE BOOL "")
set(CLANG_BOOTSTRAP_PASSTHROUGH CLANG_ORDER_FILE LLVM_PROFDATA_FILE CACHE STRING "")

set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -flto -gline-tables-only -fno-stack-protector -fno-common -DNDEBUG -Wno-profile-instr-unprofiled" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -flto -gline-tables-only -fno-stack-protector -fno-common -DNDEBUG -Wno-profile-instr-unprofiled" CACHE STRING "")
set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "")

set(LIBCXX_INSTALL_LIBRARY OFF CACHE BOOL "")
set(LIBCXX_INSTALL_HEADERS OFF CACHE BOOL "")
set(LLVM_LTO_VERSION_OFFSET 3000 CACHE STRING "")

# setup toolchain
set(LLVM_INSTALL_TOOLCHAIN_ONLY ON CACHE BOOL "")
set(LLVM_TOOLCHAIN_TOOLS
  llvm-dsymutil
  llvm-cov
  llvm-dwarfdump
  llvm-profdata
  llvm-objdump
  llvm-nm
  llvm-size
  CACHE STRING "")

# test args

set(LLVM_LIT_ARGS "--xunit-xml-output=testresults.xunit.xml -v" CACHE STRING "")
