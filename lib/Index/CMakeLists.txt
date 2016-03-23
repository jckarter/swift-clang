set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_library(clangIndex
  CodegenNameGenerator.cpp
  CommentToXML.cpp
  FileIndexRecord.cpp
  IndexBody.cpp
  IndexDataStore.cpp
  IndexDecl.cpp
  IndexingAction.cpp
  IndexingContext.cpp
  IndexRecordHasher.cpp
  IndexRecordReader.cpp
  ClangIndexRecordWriter.cpp
  IndexSymbol.cpp
  IndexTypeSourceInfo.cpp
  IndexUnitReader.cpp
  IndexUnitWriter.cpp
  USRGeneration.cpp

  ADDITIONAL_HEADERS
  IndexingContext.h
  SimpleFormatContext.h

  LINK_LIBS
  clangAST
  clangBasic
  clangFormat
  clangFrontend
  clangRewrite
  clangToolingCore
  )
