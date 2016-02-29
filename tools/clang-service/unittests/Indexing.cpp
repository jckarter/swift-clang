#include "clang-service/Service.h"
#include "clang-service/Support/InProcessClient.h"
#include "clang-service/Support/ValueUtils.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"

#include "gtest/gtest.h"

using namespace ClangService;
using namespace llvm;

namespace {

struct IndexingTest : ::testing::Test {
  std::unique_ptr<Client> C;

#define REGISTER_UID(Name, UIDStr) LazyCSUID Name;
#include "clang-service/ProtocolUIDs.inc"

  void SetUp() {
    C = make_unique<InProcessClient>();

#define REGISTER_UID(Name, UIDStr) Name = {UIDStr};
#include "clang-service/ProtocolUIDs.inc"
  }
};

TEST_F(IndexingTest, Simple) {
  int CCFd;
  SmallString<128> Path;
  auto EC = sys::fs::createTemporaryFile("simple", "cpp", CCFd, Path);
  if (EC)
    return;
  tool_output_file TOF{Path.c_str(), CCFd};
  TOF.os() << "struct S { int x; };    \n";
  TOF.os() << "int f(S s) { return s.x; }";
  TOF.os().flush();

  Value R1 =
      C->request(Value::dict({{KeyRequest, RequestIndexSource.c_str()},
                              {KeyName, Path.c_str()},
                              {KeyCmdArgs, toStringArray({Path.c_str()})},
                              {KeyIndexExcludeDeclsFromPCH, true},
                              {KeyIndexDiagnostics, true}}));
}

} // end anonymous namespace
