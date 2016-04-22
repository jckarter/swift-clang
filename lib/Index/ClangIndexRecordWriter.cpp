//===--- ClangIndexRecordWriter.cpp - Index record serialization ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ClangIndexRecordWriter.h"
#include "FileIndexRecord.h"
#include "clang/Index/IndexSymbol.h"
#include "clang/Index/IndexRecordReader.h"
#include "clang/Index/USRGeneration.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclObjC.h"

using namespace clang;
using namespace clang::index;

StringRef ClangIndexRecordWriter::getUSR(const Decl *D) {
  assert(D->isCanonicalDecl());
  auto Insert = USRByDecl.insert(std::make_pair(D, StringRef()));
  if (Insert.second) {
    Insert.first->second = getUSRNonCached(D);
  }
  return Insert.first->second;
}

StringRef ClangIndexRecordWriter::getUSRNonCached(const Decl *D) {
  SmallString<256> Buf;
  bool Ignore = generateUSRForDecl(D, Buf);
  if (Ignore)
    return StringRef();
  StringRef USR = Buf.str();
  char *Ptr = Allocator.Allocate<char>(USR.size());
  std::copy(USR.begin(), USR.end(), Ptr);
  return StringRef(Ptr, USR.size());
}

ClangIndexRecordWriter::ClangIndexRecordWriter(ASTContext &Ctx,
                                               RecordingOptions Opts)
    : Impl(Opts.DataDirPath), Ctx(Ctx), RecordOpts(std::move(Opts)),
      Hasher(Ctx) {
  if (Opts.RecordSymbolCodeGenName)
    CGNameGen.reset(new CodegenNameGenerator(Ctx));
}

ClangIndexRecordWriter::~ClangIndexRecordWriter() {}

static indexstore_symbol_kind_t getIndexStoreKind(SymbolKind K) {
  switch (K) {
  case SymbolKind::Unknown:
    return INDEXSTORE_SYMBOL_KIND_UNKNOWN;
  case SymbolKind::Module:
    return INDEXSTORE_SYMBOL_KIND_MODULE;
  case SymbolKind::Namespace:
    return INDEXSTORE_SYMBOL_KIND_NAMESPACE;
  case SymbolKind::NamespaceAlias:
    return INDEXSTORE_SYMBOL_KIND_NAMESPACEALIAS;
  case SymbolKind::Macro:
    return INDEXSTORE_SYMBOL_KIND_MACRO;
  case SymbolKind::Enum:
    return INDEXSTORE_SYMBOL_KIND_ENUM;
  case SymbolKind::Struct:
    return INDEXSTORE_SYMBOL_KIND_STRUCT;
  case SymbolKind::Class:
    return INDEXSTORE_SYMBOL_KIND_CLASS;
  case SymbolKind::Protocol:
    return INDEXSTORE_SYMBOL_KIND_PROTOCOL;
  case SymbolKind::Extension:
    return INDEXSTORE_SYMBOL_KIND_EXTENSION;
  case SymbolKind::Union:
    return INDEXSTORE_SYMBOL_KIND_UNION;
  case SymbolKind::TypeAlias:
    return INDEXSTORE_SYMBOL_KIND_TYPEALIAS;
  case SymbolKind::Function:
    return INDEXSTORE_SYMBOL_KIND_FUNCTION;
  case SymbolKind::Variable:
    return INDEXSTORE_SYMBOL_KIND_VARIABLE;
  case SymbolKind::Field:
    return INDEXSTORE_SYMBOL_KIND_FIELD;
  case SymbolKind::EnumConstant:
    return INDEXSTORE_SYMBOL_KIND_ENUMCONSTANT;
  case SymbolKind::InstanceMethod:
    return INDEXSTORE_SYMBOL_KIND_INSTANCEMETHOD;
  case SymbolKind::ClassMethod:
    return INDEXSTORE_SYMBOL_KIND_CLASSMETHOD;
  case SymbolKind::StaticMethod:
    return INDEXSTORE_SYMBOL_KIND_STATICMETHOD;
  case SymbolKind::InstanceProperty:
    return INDEXSTORE_SYMBOL_KIND_INSTANCEPROPERTY;
  case SymbolKind::ClassProperty:
    return INDEXSTORE_SYMBOL_KIND_CLASSPROPERTY;
  case SymbolKind::StaticProperty:
    return INDEXSTORE_SYMBOL_KIND_STATICPROPERTY;
  case SymbolKind::Constructor:
    return INDEXSTORE_SYMBOL_KIND_CONSTRUCTOR;
  case SymbolKind::Destructor:
    return INDEXSTORE_SYMBOL_KIND_DESTRUCTOR;
  case SymbolKind::ConversionFunction:
    return INDEXSTORE_SYMBOL_KIND_CONVERSIONFUNCTION;
  }
  llvm_unreachable("unexpected symbol kind");
}

static indexstore_symbol_sub_kind_t
getIndexStoreSubKinds(SymbolSubKindSet SubKinds) {
  return (indexstore_symbol_sub_kind_t)SubKinds;
}

static indexstore_symbol_language_t getIndexStoreLang(SymbolLanguage L) {
  switch (L) {
  case SymbolLanguage::C:
    return INDEXSTORE_SYMBOL_LANG_C;
  case SymbolLanguage::ObjC:
    return INDEXSTORE_SYMBOL_LANG_OBJC;
  case SymbolLanguage::CXX:
    return INDEXSTORE_SYMBOL_LANG_CXX;
  }
  llvm_unreachable("unexpected symbol language");
}

bool ClangIndexRecordWriter::writeRecord(StringRef Filename,
                                         const FileIndexRecord &IdxRecord,
                                         std::string &Error,
                                         std::string *OutRecordFile) {

  auto RecordHash = Hasher.hashRecord(IdxRecord);

  switch (Impl.beginRecord(Filename, RecordHash, Error, OutRecordFile)) {
  case IndexRecordWriter::Result::Success:
    break; // Continue writing.
  case IndexRecordWriter::Result::Failure:
    return true;
  case IndexRecordWriter::Result::AlreadyExists:
    return false;
  }

  ASTContext &Ctx = getASTContext();
  SourceManager &SM = Ctx.getSourceManager();
  FileID FID = IdxRecord.getFileID();
  auto getLineCol = [&](unsigned Offset) -> std::pair<unsigned, unsigned> {
    unsigned LineNo = SM.getLineNumber(FID, Offset);
    unsigned ColNo = SM.getColumnNumber(FID, Offset);
    return std::make_pair(LineNo, ColNo);
  };

  for (auto &Occur : IdxRecord.getDeclOccurrences()) {
    unsigned Line, Col;
    std::tie(Line, Col) = getLineCol(Occur.Offset);
    SmallVector<writer::SymbolRelation, 3> Related;
    Related.reserve(Occur.Relations.size());
    for (auto &Rel : Occur.Relations)
      Related.push_back(writer::SymbolRelation{Rel.RelatedSymbol, Rel.Roles});

    Impl.addOccurrence(Occur.Dcl, Occur.Roles, Line, Col, Related);
  }

  PrintingPolicy Policy(Ctx.getLangOpts());
  Policy.SuppressTemplateArgsInCXXConstructors = true;

  auto Result = Impl.endRecord(Error,
      [&](writer::OpaqueDecl OD, SmallVectorImpl<char> &Scratch) {
    const Decl *D = static_cast<const Decl *>(OD);
    auto Info = getSymbolInfo(D);

    writer::Symbol Sym;
    Sym.Kind = getIndexStoreKind(Info.Kind);
    Sym.Lang = getIndexStoreLang(Info.Lang);
    Sym.SubKinds = getIndexStoreSubKinds(Info.SubKinds);

    auto *ND = dyn_cast<NamedDecl>(D);
    if (ND) {
      llvm::raw_svector_ostream OS(Scratch);
      DeclarationName DeclName = ND->getDeclName();
      if (!DeclName.isEmpty())
        DeclName.print(OS, Policy);
    }
    unsigned NameLen = Scratch.size();
    Sym.Name = StringRef(Scratch.data(), NameLen);

    Sym.USR = getUSR(D);
    assert(!Sym.USR.empty() && "Recorded decl without USR!");

    if (CGNameGen && ND) {
      llvm::raw_svector_ostream OS(Scratch);
      CGNameGen->writeName(ND, OS);
    }
    unsigned CGNameLen = Scratch.size() - NameLen;
    Sym.CodeGenName = StringRef(Scratch.data() + NameLen, CGNameLen);
    return Sym;
  });

  switch (Result) {
  case IndexRecordWriter::Result::Success:
  case IndexRecordWriter::Result::AlreadyExists:
    return false;
  case IndexRecordWriter::Result::Failure:
    return true;
  }
}
