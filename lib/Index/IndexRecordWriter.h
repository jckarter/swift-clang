//===--- IndexRecordWriter.h - Index record serialization -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_INDEX_INDEXRECORDWRITER_H
#define LLVM_CLANG_LIB_INDEX_INDEXRECORDWRITER_H

#include "IndexRecordHasher.h"
#include "clang/Index/IndexingAction.h"
#include "clang/Index/CodegenNameGenerator.h"
#include "llvm/ADT/SmallString.h"

namespace clang {
  class ASTContext;
  class Decl;

namespace index {
  class FileIndexRecord;

class IndexRecordWriter {
  ASTContext &Ctx;
  RecordingOptions RecordOpts;

  SmallString<64> RecordsPath;
  std::unique_ptr<CodegenNameGenerator> CGNameGen;
  llvm::BumpPtrAllocator Allocator;
  llvm::DenseMap<const Decl *, StringRef> USRByDecl;
  IndexRecordHasher Hasher;

public:
  IndexRecordWriter(ASTContext &Ctx, RecordingOptions Opts);
  ~IndexRecordWriter();

  ASTContext &getASTContext() { return Ctx; }
  CodegenNameGenerator *getCGNameGen() { return CGNameGen.get(); }

  bool writeRecord(StringRef Filename, const FileIndexRecord &Record,
                   std::string &Error, std::string *RecordFile = nullptr);
  StringRef getUSR(const Decl *D);

private:
  StringRef getUSRNonCached(const Decl *D);
};

} // end namespace index
} // end namespace clang

#endif
