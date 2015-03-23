set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_unittest(DynamicASTMatchersTests
  VariantValueTest.cpp
  ParserTest.cpp
  RegistryTest.cpp)

target_link_libraries(DynamicASTMatchersTests
  clangAST
  clangASTMatchers
  clangBasic
  clangCodeGen
  clangDynamicASTMatchers
  clangFrontend
  clangTooling
  )
