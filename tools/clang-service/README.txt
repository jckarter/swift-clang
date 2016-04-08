=============
clang-service
=============

Project layout
==============

Headers: clang/include/clang-service
Libraries, tests, etc.: clang/tools/clang-service (current directory)

Building the project
====================

Standard llvm+clang cmake instructions.

Testing
=======

First, run:

  ninja ClangServiceUnitTests c-index-test
  ninja clang (Optional)

You can run the unit tests like this:

  ./tools/clang/tools/clang-service/unittests/ClangServiceTests

You can run `ninja check-clang` for the libclang tests.
