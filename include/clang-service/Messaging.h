#ifndef LLVM_CLANG_CLANGSERVICE_MESSAGING_H
#define LLVM_CLANG_CLANGSERVICE_MESSAGING_H

namespace ClangService {

// FIXME: Share definitions with sourcekitd/Internal-XPC.h.
static const char *KeyInternalMsg = "internal_msg";
static const char *KeySemaEditorDelay = "semantic_editor_delay";
static const char *KeyTracingEnabled = "tracing_enabled";
static const char *KeyMsgResponse = "response";

enum class Message : char {
  Initialization,
  Notification,
  UIDSynchronization,
  TraceMessage
};

enum class ErrorKind : char {
  RequestInvalid,
  RequestFailed,
  RequestInterrupted,
  RequestCancelled,
  Fatal,
  NoError
};

} // end namespace ClangService

#endif // LLVM_CLANG_CLANGSERVICE_MESSAGING_H
