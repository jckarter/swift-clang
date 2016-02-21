//===--- IndexRecordReader.cpp - Index record deserialization -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Index/IndexRecordReader.h"
#include "IndexDataStoreUtils.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Bitcode/BitstreamReader.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::index;
using namespace clang::index::store;
using namespace llvm;

namespace {

/// Helper class that saves the current stream position and
/// then restores it when destroyed.
struct SavedStreamPosition {
  explicit SavedStreamPosition(llvm::BitstreamCursor &Cursor)
    : Cursor(Cursor), Offset(Cursor.GetCurrentBitNo()) { }

  ~SavedStreamPosition() {
    Cursor.JumpToBit(Offset);
  }

private:
  llvm::BitstreamCursor &Cursor;
  uint64_t Offset;
};

enum class StreamVisit {
  Continue,
  Skip,
  Abort
};

template <typename ImplClass>
class BitstreamVisitor {
  SmallVector<unsigned, 4> BlockStack;

protected:
  BitstreamCursor &Stream;
  std::string *Error;

public:
  BitstreamVisitor(BitstreamCursor &Stream)
    : Stream(Stream) {}

  StreamVisit visitBlock(unsigned ID) {
    return StreamVisit::Continue;
  }

  bool visit(std::string &Error) {
    this->Error = &Error;

    RecordData Record;
    while (1) {
      llvm::BitstreamEntry Entry = Stream.advance(BitstreamCursor::AF_DontPopBlockAtEnd);

      switch (Entry.Kind) {
      case llvm::BitstreamEntry::Error:
        Error = "malformed serialization";
        return false;

      case llvm::BitstreamEntry::EndBlock:
        if (BlockStack.empty())
          return true;
        BlockStack.pop_back();
        if (Stream.ReadBlockEnd()) {
          Error = "malformed serialization";
          return false;
        }
        if (Stream.AtEndOfStream())
          return true;
        break;

      case llvm::BitstreamEntry::SubBlock: {
        if (Entry.ID == llvm::bitc::BLOCKINFO_BLOCK_ID) {
          if (Stream.ReadBlockInfoBlock()) {
            Error = "malformed BlockInfoBlock";
            return false;
          }
          break;
        }

        StreamVisit Ret = static_cast<ImplClass*>(this)->visitBlock(Entry.ID);
        switch (Ret) {
        case StreamVisit::Continue:
          if (Stream.EnterSubBlock(Entry.ID)) {
            Error = "malformed block record";
            return false;
          }
          readBlockAbbrevs(Stream);
          BlockStack.push_back(Entry.ID);
          break;

        case StreamVisit::Skip: 
          if (Stream.SkipBlock()) {
            Error = "malformed serialization";
            return false;
          }
          if (Stream.AtEndOfStream())
            return true;
          break;

        case StreamVisit::Abort:
          return false;
        }
        break;
      }

      case llvm::BitstreamEntry::Record: {
        Record.clear();
        StringRef Blob;
        unsigned RecID = Stream.readRecord(Entry.ID, Record, &Blob);
        unsigned BlockID = BlockStack.empty() ? 0 : BlockStack.back();
        StreamVisit Ret = static_cast<ImplClass*>(this)->visitRecord(BlockID, RecID, Record, Blob);
        switch (Ret) {
        case StreamVisit::Continue:
          break;

        case StreamVisit::Skip: 
          Stream.skipRecord(Entry.ID);
          break;

        case StreamVisit::Abort:
          return false;
        }
        break;
      }
      }
    }
  }

  static void readBlockAbbrevs(llvm::BitstreamCursor &Cursor) {
    while (true) {
      uint64_t Offset = Cursor.GetCurrentBitNo();
      unsigned Code = Cursor.ReadCode();

      // We expect all abbrevs to be at the start of the block.
      if (Code != llvm::bitc::DEFINE_ABBREV) {
        Cursor.JumpToBit(Offset);
        return;
      }
      Cursor.ReadAbbrevRecord();
    }
  }
};

} // anonymous namespace

struct IndexRecordReader::Implementation {
  BumpPtrAllocator Allocator;
  std::unique_ptr<MemoryBuffer> Buffer;
  llvm::BitstreamReader BitReader;
  llvm::BitstreamCursor DeclCursor;
  llvm::BitstreamCursor OccurCursor;
  ArrayRef<uint32_t> DeclOffsets;
  const IndexRecordDecl **Decls;

  void setDeclOffsets(ArrayRef<uint32_t> Offs) {
    DeclOffsets = Offs;
    Decls = Allocator.Allocate<const IndexRecordDecl*>(Offs.size());
    memset(Decls, 0, sizeof(IndexRecordDecl*)*Offs.size());
  }

  unsigned getNumDecls() const { return DeclOffsets.size(); }

  const IndexRecordDecl *getDeclByID(unsigned DeclID) {
    if (DeclID == 0)
      return nullptr;
    return getDecl(DeclID-1);
  }

  const IndexRecordDecl *getDecl(unsigned Index) {
    assert(Index < getNumDecls());
    if (const IndexRecordDecl *D = Decls[Index])
      return D;

    IndexRecordDecl *D = Allocator.Allocate<IndexRecordDecl>();
    readDecl(Index, *D);
    Decls[Index] = D;
    return D;
  }

  /// Goes through the decls and populates a vector of record decls, based on
  /// what the given function returns.
  ///
  /// The advantage of this function is to allocate memory only for the record
  /// decls that the caller is interested in.
  void searchDecls(llvm::function_ref<DeclSearchCheck> Checker,
                   SmallVectorImpl<const IndexRecordDecl *> &FoundDecls) {
    for (unsigned I = 0, E = getNumDecls(); I != E; ++I) {
      if (const IndexRecordDecl *D = Decls[I]) {
        DeclSearchReturn Ret = Checker(*D);
        if (Ret.AcceptDecl)
          FoundDecls.push_back(D);
        if (!Ret.ContinueSearch)
          return;
        continue;
      }

      IndexRecordDecl LocalD;
      readDecl(I, LocalD);
      DeclSearchReturn Ret = Checker(LocalD);
      if (Ret.AcceptDecl) {
        IndexRecordDecl *D = Allocator.Allocate<IndexRecordDecl>();
        *D = LocalD;
        Decls[I] = D;
        FoundDecls.push_back(D);
      }
      if (!Ret.ContinueSearch)
        return;
    }
  }

  void readDecl(unsigned Index, IndexRecordDecl &RecD) {
    RecordData Record;
    StringRef Blob;
    DeclCursor.JumpToBit(DeclOffsets[Index]);
    unsigned Code = DeclCursor.ReadCode();
    unsigned RecID = DeclCursor.readRecord(Code, Record, &Blob);
    assert(RecID == REC_DECLINFO);
    (void)RecID;

    unsigned I = 0;
    RecD.DeclID = Index+1;
    RecD.Kind = (SymbolKind)read(Record, I);
    RecD.Lang = (SymbolLanguage)read(Record, I);
    RecD.CXXTemplateKind = (SymbolCXXTemplateKind)read(Record, I);
    RecD.Roles = read(Record, I);
    RecD.RelatedRoles = read(Record, I);
    size_t NameLen = read(Record, I);
    size_t USRLen = read(Record, I);
    RecD.Name = Blob.substr(0, NameLen);
    RecD.USR = Blob.substr(NameLen, USRLen);
    RecD.CodeGenName = Blob.substr(NameLen+USRLen);
  }

  /// Reads occurrence data.
  /// \param DeclsFilter if non-empty indicates the list of decls that we want
  /// to get occurrences for. If empty then indicates that we want occurrences
  /// for all decls.
  /// \param RelatedDeclsFilter Same as \c DeclsFilter but for related decls.
  /// \returns true if the occurrence info was filled out, false if occurrence
  /// was ignored.
  bool readOccurrence(RecordDataImpl &Record, StringRef Blob,
                      ArrayRef<const IndexRecordDecl *> DeclsFilter,
                      ArrayRef<const IndexRecordDecl *> RelatedDeclsFilter,
                      IndexRecordOccurrence &RecOccur) {

    auto isDeclIDContained = [](unsigned DeclID,
                                ArrayRef<const IndexRecordDecl *> Ds) -> bool {
      if (Ds.empty())
        return true; // empty means accept all.
      auto pred = [DeclID](const IndexRecordDecl *D) { return D->DeclID == DeclID; };
      return std::find_if(Ds.begin(), Ds.end(), pred) != Ds.end();
    };

    unsigned I = 0;
    unsigned DeclID = read(Record, I);
    if (!isDeclIDContained(DeclID, DeclsFilter))
      return false;

    if (!RelatedDeclsFilter.empty()) {
      unsigned RelI = I+3;
      unsigned NumRelated = read(Record, RelI);
      bool FoundRelated = false;
      while (NumRelated--) {
        ++RelI; // roles;
        unsigned RelDID = read(Record, RelI);
        if (isDeclIDContained(RelDID, RelatedDeclsFilter)) {
          FoundRelated = true;
          break;
        }
      }
      if (!FoundRelated)
        return false;
    }

    RecOccur.Dcl = getDeclByID(DeclID);
    RecOccur.Roles = read(Record, I);
    RecOccur.Line = read(Record, I);
    RecOccur.Column = read(Record, I);

    unsigned NumRelated = read(Record, I);
    while (NumRelated--) {
      SymbolRole RelRole = (SymbolRole)read(Record, I);
      const IndexRecordDecl *RelD = getDeclByID(read(Record, I));
      RecOccur.Relations.emplace_back((unsigned)RelRole, RelD);
    }

    return true;
  }

  bool foreachDecl(function_ref<bool(const IndexRecordDecl *)> Receiver) {
    for (unsigned i = 0, e = getNumDecls(); i != e; ++i) {
      if (!Receiver(getDecl(i)))
        return false;
    }
    return true;
  }

  bool foreachOccurrence(ArrayRef<const IndexRecordDecl *> DeclsFilter,
                         ArrayRef<const IndexRecordDecl *> RelatedDeclsFilter,
                         function_ref<bool(const IndexRecordOccurrence &)> Receiver) {
    class OccurBitVisitor : public BitstreamVisitor<OccurBitVisitor> {
      IndexRecordReader::Implementation &Reader;
      ArrayRef<const IndexRecordDecl *> DeclsFilter;
      ArrayRef<const IndexRecordDecl *> RelatedDeclsFilter;
      function_ref<bool(const IndexRecordOccurrence &)> Receiver;

    public:
      OccurBitVisitor(llvm::BitstreamCursor &Stream,
                      IndexRecordReader::Implementation &Reader,
                      ArrayRef<const IndexRecordDecl *> DeclsFilter,
                      ArrayRef<const IndexRecordDecl *> RelatedDeclsFilter,
                      function_ref<bool(const IndexRecordOccurrence &)> Receiver)
        : BitstreamVisitor(Stream),
          Reader(Reader),
          DeclsFilter(DeclsFilter),
          RelatedDeclsFilter(RelatedDeclsFilter),
          Receiver(std::move(Receiver)) {}

      StreamVisit visitRecord(unsigned BlockID, unsigned RecID,
                              RecordDataImpl &Record, StringRef Blob) {
        assert(RecID == REC_DECLOCCURRENCE);
        IndexRecordOccurrence RecOccur;
        if (Reader.readOccurrence(Record, Blob, DeclsFilter, RelatedDeclsFilter,
                                   RecOccur))
          if (!Receiver(RecOccur))
            return StreamVisit::Abort;
        return StreamVisit::Continue;
      }
    };

    SavedStreamPosition SavedPosition(OccurCursor);
    OccurBitVisitor Visitor(OccurCursor, *this, DeclsFilter, RelatedDeclsFilter,
                            Receiver);
    std::string Error;
    return Visitor.visit(Error);
  }

  static uint64_t read(RecordDataImpl &Record, unsigned &I) {
    return Record[I++];
  }
};

namespace {

class IndexBitstreamVisitor : public BitstreamVisitor<IndexBitstreamVisitor> {
  IndexRecordReader::Implementation &Reader;

public:
  IndexBitstreamVisitor(llvm::BitstreamCursor &Stream,
                        IndexRecordReader::Implementation &Reader)
    : BitstreamVisitor(Stream), Reader(Reader) {}

  StreamVisit visitBlock(unsigned ID) {
    switch ((BitBlock)ID) {
    case VERSION_BLOCK_ID:
    case DECLOFFSETS_BLOCK_ID:
      return StreamVisit::Continue;

    case DECLS_BLOCK_ID:
      Reader.DeclCursor = Stream;
      if (Reader.DeclCursor.EnterSubBlock(ID)) {
        *Error = "malformed block record";
        return StreamVisit::Abort;
      }
      readBlockAbbrevs(Reader.DeclCursor);
      return StreamVisit::Skip;

    case DECLOCCURRENCES_BLOCK_ID:
      Reader.OccurCursor = Stream;
      if (Reader.OccurCursor.EnterSubBlock(ID)) {
        *Error = "malformed block record";
        return StreamVisit::Abort;
      }
      readBlockAbbrevs(Reader.OccurCursor);
      return StreamVisit::Skip;
    }

    // Some newly introduced block in a minor version update that we cannot
    // handle.
    return StreamVisit::Skip;
  }

  StreamVisit visitRecord(unsigned BlockID, unsigned RecID,
                          RecordDataImpl &Record, StringRef Blob) {
    switch (BlockID) {
    case VERSION_BLOCK_ID: {
      unsigned StoreFormatVersion = Record[0];
      if (StoreFormatVersion != STORE_FORMAT_VERSION) {
        llvm::raw_string_ostream OS(*Error);
        OS << "Store format version mismatch: " << StoreFormatVersion;
        OS << " , expected: " << STORE_FORMAT_VERSION;
        return StreamVisit::Abort;
      }
      break;
    }
    case DECLOFFSETS_BLOCK_ID:
      assert(RecID == REC_DECLOFFSETS);
      Reader.setDeclOffsets(makeArrayRef((uint32_t*)Blob.data(), Record[0]));
      break;

    case DECLS_BLOCK_ID:
    case DECLOCCURRENCES_BLOCK_ID:
      llvm_unreachable("shouldn't visit this block'");
    }
    return StreamVisit::Continue;
  }
};

} // anonymous namespace

std::unique_ptr<IndexRecordReader>
IndexRecordReader::createWithRecordFilename(StringRef RecordFilename,
                                            StringRef StorePath,
                                            std::string &Error) {
  SmallString<128> PathBuf = StorePath;
  makeRecordSubDir(PathBuf);
  sys::path::append(PathBuf, RecordFilename);
  return createWithFilePath(PathBuf.str(), Error);
}

std::unique_ptr<IndexRecordReader>
IndexRecordReader::createWithFilePath(StringRef FilePath, std::string &Error) {
  auto ErrOrBuf = MemoryBuffer::getFile(FilePath, /*FileSize=*/-1,
                                        /*RequiresNullTerminator=*/false);
  if (!ErrOrBuf) {
    raw_string_ostream(Error) << "failed opening index record '"
      << FilePath << "': " << ErrOrBuf.getError().message();
    return nullptr;
  }
  return createWithBuffer(std::move(*ErrOrBuf), Error);
}

std::unique_ptr<IndexRecordReader>
IndexRecordReader::createWithBuffer(std::unique_ptr<llvm::MemoryBuffer> Buffer,
                                    std::string &Error) {

  std::unique_ptr<IndexRecordReader> Reader;
  Reader.reset(new IndexRecordReader());
  auto &Impl = Reader->Impl;
  Impl.Buffer = std::move(Buffer);
  Impl.BitReader.init((const unsigned char *)Impl.Buffer->getBufferStart(),
                      (const unsigned char *)Impl.Buffer->getBufferEnd());

  llvm::BitstreamCursor Stream(Impl.BitReader);

  // Sniff for the signature.
  if (Stream.Read(8) != 'C' ||
      Stream.Read(8) != 'I' ||
      Stream.Read(8) != 'D' ||
      Stream.Read(8) != 'X') {
    Error = "not a serialized clang index record file";
    return nullptr;
  }

  IndexBitstreamVisitor BitVisitor(Stream, Impl);
  if (!BitVisitor.visit(Error))
    return nullptr;

  return Reader;
}

IndexRecordReader::IndexRecordReader()
  : Impl(*new Implementation()) {

}

IndexRecordReader::~IndexRecordReader() {
  delete &Impl;
}

void IndexRecordReader::searchDecls(
                        llvm::function_ref<DeclSearchCheck> Checker,
                        SmallVectorImpl<const IndexRecordDecl *> &FoundDecls) {
  return Impl.searchDecls(std::move(Checker), FoundDecls);
}

bool IndexRecordReader::foreachDecl(function_ref<bool(const IndexRecordDecl *)> Receiver) {
  return Impl.foreachDecl(std::move(Receiver));
}

bool IndexRecordReader::foreachOccurrence(
                  ArrayRef<const IndexRecordDecl *> DeclsFilter,
                  ArrayRef<const IndexRecordDecl *> RelatedDeclsFilter,
                  function_ref<bool(const IndexRecordOccurrence &)> Receiver) {
  return Impl.foreachOccurrence(DeclsFilter, RelatedDeclsFilter,
                                std::move(Receiver));
}

bool IndexRecordReader::foreachOccurrence(
            llvm::function_ref<bool(const IndexRecordOccurrence &)> Receiver) {
  return foreachOccurrence(None, None, std::move(Receiver));
}
