#include "clang-service/Service.h"
#include "clang-service/Support/InProcessClient.h"
#include "clang-service/Support/ValueUtils.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"

#include "gtest/gtest.h"

using namespace ClangService;
using namespace llvm;

namespace {

struct CodeCompletionTest : ::testing::Test {
  std::unique_ptr<Client> C;

#define REGISTER_UID(Name, UIDStr) LazyCSUID Name;
#include "clang-service/ProtocolUIDs.inc"

  void SetUp() {
    C = make_unique<InProcessClient>();

#define REGISTER_UID(Name, UIDStr) Name = {UIDStr};
#include "clang-service/ProtocolUIDs.inc"
  }

  bool isOptionalInt64(Value &V) { return V.isNull() || V.isInt64(); }

  bool isOptionalString(Value &V) { return V.isNull() || V.isStringLike(); }

  void isCXCodeCompleteResults(Value &V) {
    auto &D = V.getDict();
    for (auto &V : D[KeyCodeCompleteResults].getArray())
      isCXCompletionResult(V);
    EXPECT_TRUE(D[KeyToken].isInt64());
    EXPECT_TRUE(D[KeyCodeCompleteContexts].isInt64());
    EXPECT_TRUE(D[KeyCodeCompleteContainerKind].isInt64());
    EXPECT_TRUE(D[KeyCodeCompleteContainerIncomplete].isBool());
    EXPECT_TRUE(isOptionalString(D[KeyCodeCompleteContainerUSR]));
    EXPECT_TRUE(isOptionalString(D[KeyCodeCompleteObjCSelector]));
    if (D[KeyCodeCompleteDiagnostics].isArray())
      for (auto &VDiagnostic : D[KeyCodeCompleteDiagnostics].getArray())
        isCXDiagnostic(VDiagnostic);
  }

  void isCXCompletionResult(Value &V) {
    auto &D = V.getDict();
    EXPECT_TRUE(D[KeyCodeCompleteCompletionKind].isInt64());
    isCXCompletionString(D[KeyCodeCompleteCompletionStr]);
  }

  void isCXCompletionString(Value &V) {
    auto &D = V.getDict();
    Value &Kind = D[KeyCodeCompleteCompletionStrKind];
    Value &Text = D[KeyCodeCompleteCompletionStrText];
    Value &Chunks = D[KeyCodeCompleteCompletionStrChunks];
    Value &Parent = D[KeyCodeCompleteCompletionStrParent];
    Value &Brief = D[KeyCodeCompleteCompletionStrBrief];
    EXPECT_TRUE(isOptionalInt64(Kind));
    EXPECT_TRUE(isOptionalString(Text));
    for (auto &V : Chunks.getArray())
      isCXCompletionString(V);
    EXPECT_TRUE(D[KeyCodeCompleteCompletionStrAvailability].isInt64());
    EXPECT_TRUE(D[KeyCodeCompleteCompletionStrPriority].isInt64());
    EXPECT_TRUE(D[KeyCodeCompleteCompletionStrAnnotations].isArray());
    EXPECT_TRUE(isOptionalString(Parent));
    EXPECT_TRUE(D[KeyCodeCompleteCompletionStrParentKind].isInt64());
    EXPECT_TRUE(isOptionalString(Brief));
  }

  void isCXDiagnostic(Value &V) {
    auto &D = V.getDict();
    EXPECT_TRUE(D[KeyDiagnosticSeverity].isInt64());
    EXPECT_TRUE(D[KeyDiagnosticString].isStringLike());
    isCXSourceLocation(D[KeyDiagnosticLocation]);
    auto &AFixits = D[KeyDiagnosticFixits].getArray();
    for (auto &VFixit : AFixits) {
      EXPECT_TRUE(VFixit.getArray()[0].isStringLike());
      isCXSourceRange(VFixit.getArray()[1]);
    }
  }

  void isCXSourceLocation(Value &V) {
    auto &D = V.getDict();
    EXPECT_TRUE(D[KeyLocationInSysHeader].isBool());
    EXPECT_TRUE(D[KeyLocationFromMainFile].isBool());
    isCXFile(D[KeyLocationExpansionFile]);
    EXPECT_TRUE(D[KeyLocationExpansionLineOffset].isInt64());
    EXPECT_TRUE(D[KeyLocationExpansionColOffset].isInt64());
    EXPECT_TRUE(D[KeyLocationExpansionOffset].isInt64());
    EXPECT_TRUE(D[KeyLocationPresumedFilename].isStringLike());
    EXPECT_TRUE(D[KeyLocationPresumedLineOffset].isInt64());
    EXPECT_TRUE(D[KeyLocationPresumedColOffset].isInt64());
    isCXFile(D[KeyLocationInstantiationFile]);
    EXPECT_TRUE(D[KeyLocationInstantiationLineOffset].isInt64());
    EXPECT_TRUE(D[KeyLocationInstantiationColOffset].isInt64());
    EXPECT_TRUE(D[KeyLocationInstantiationOffset].isInt64());
    isCXFile(D[KeyLocationSpellingFile]);
    EXPECT_TRUE(D[KeyLocationSpellingLineOffset].isInt64());
    EXPECT_TRUE(D[KeyLocationSpellingColOffset].isInt64());
    EXPECT_TRUE(D[KeyLocationSpellingOffset].isInt64());
    isCXFile(D[KeyLocationFileFile]);
    EXPECT_TRUE(D[KeyLocationFileLineOffset].isInt64());
    EXPECT_TRUE(D[KeyLocationFileColOffset].isInt64());
    EXPECT_TRUE(D[KeyLocationFileOffset].isInt64());
  }

  void isCXSourceRange(Value &V) {
    auto &D = V.getDict();
    isCXSourceLocation(D[KeyRangeStart]);
    isCXSourceLocation(D[KeyRangeEnd]);
  }

  void isCXFile(Value &V) {
    auto &D = V.getDict();
    EXPECT_TRUE(D[KeyLocationFileName].isStringLike());
    EXPECT_TRUE(D[KeyLocationFileUID].isDataLike());
  }
};

TEST_F(CodeCompletionTest, Simple) {
  TimerGroup TG{"CodeCompletionTest/Simple"};
  Timer FileCreationTimer{"File Creation", TG};
  Timer RequestTimer{"Request Processing", TG};

  FileCreationTimer.startTimer();
  int CCFd;
  SmallString<128> Path;
  auto EC = sys::fs::createTemporaryFile("simple", "c", CCFd, Path);
  if (EC)
    return;
  tool_output_file TOF{Path.c_str(), CCFd};
  TOF.os() << "int main() { return 0; }";
  TOF.os().flush();
  FileCreationTimer.stopTimer();

  RequestTimer.startTimer();
  Value OpenResp =
      C->request(Value::dict({{KeyRequest, RequestCodeCompleteOpen.c_str()},
                              {KeyName, Path.c_str()},
                              {KeyLineOffset, 1LL},
                              {KeyColOffset, 5LL},
                              {KeyCmdArgs, toStringArray({Path.c_str()})},
                              {KeyIndexOptions, 0LL},
                              {KeyIndexExcludeDeclsFromPCH, true},
                              {KeyIndexDiagnostics, false}}));
  RequestTimer.stopTimer();

  int64_t Token = OpenResp.getDict()[KeyToken].getInt64();

  isCXCodeCompleteResults(OpenResp);

  RequestTimer.startTimer();
  Value CloseResp = C->request(Value::dict(
      {{KeyRequest, RequestCodeCompleteClose.c_str()}, {KeyToken, Token}}));
  RequestTimer.stopTimer();

  EXPECT_TRUE(CloseResp.isNull());
}

TEST_F(CodeCompletionTest, Diagnostics) {
  int CCFd;
  SmallString<128> Path;
  auto EC = sys::fs::createTemporaryFile("diagnostics", "c", CCFd, Path);
  if (EC)
    return;
  tool_output_file TOF{Path.c_str(), CCFd};
  TOF.os() << "struct S { int x; };    \n";
  TOF.os() << "void f1(S *s) {         \n";
  TOF.os() << "  undeclared = 0;       \n";
  TOF.os() << "  s->x = 0;             \n";
  TOF.os() << "}                       \n";
  TOF.os().flush();

  Value OpenResp =
      C->request(Value::dict({{KeyRequest, RequestCodeCompleteOpen.c_str()},
                              {KeyName, Path.c_str()},
                              {KeyLineOffset, 4LL},
                              {KeyColOffset, 6LL},
                              {KeyCmdArgs, toStringArray({Path.c_str()})},
                              {KeyIndexDiagnostics, false},
                              {KeyDiagnosticsEnabled, true}}));

  int64_t Token = OpenResp.getDict()[KeyToken].getInt64();
  Value &Diagnostics = OpenResp.getDict()[KeyCodeCompleteDiagnostics];
  EXPECT_TRUE(Diagnostics.getArray().size() > 0);

  isCXCodeCompleteResults(OpenResp);

  Value CloseResp = C->request(Value::dict(
      {{KeyRequest, RequestCodeCompleteClose.c_str()}, {KeyToken, Token}}));

  EXPECT_TRUE(CloseResp.isNull());
}

} // end anonymous namespace
