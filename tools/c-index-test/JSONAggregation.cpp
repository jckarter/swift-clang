//===--- JSONAggregation.cpp - Index data aggregation in JSON format ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JSONAggregation.h"
#include "clang/Index/IndexDataStore.h"
#include "clang/Index/IndexRecordReader.h"
#include "clang/Index/IndexUnitReader.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::index;
using namespace llvm;

namespace {

typedef size_t FilePathIndex;
typedef size_t RecordIndex;
typedef size_t SymbolIndex;

struct UnitSourceInfo {
  FilePathIndex FilePath;
  SmallVector<RecordIndex, 2> AssociatedRecords;
};

struct UnitInfo {
  SmallVector<UnitSourceInfo, 8> Sources;
  FilePathIndex OutFile;
  StringRef Triple;
};

struct SymbolInfo {
  SymbolKind Kind;
  SymbolLanguage Lang;
  StringRef USR;
  StringRef Name;
  StringRef CodegenName;
  SymbolRoleSet Roles = 0;
  SymbolRoleSet RelatedRoles = 0;
};

struct SymbolRelationInfo {
  SymbolIndex RelatedSymbol;
  SymbolRoleSet Roles;
  SymbolRelationInfo(SymbolIndex relSymbol, SymbolRoleSet roles)
    : RelatedSymbol(relSymbol), Roles(roles) {}
};

struct SymbolOccurrenceInfo {
  SymbolIndex Symbol;
  SymbolRoleSet Roles = 0;
  std::vector<SymbolRelationInfo> Relations;
  unsigned Line;
  unsigned Column;
};

struct RecordInfo {
  SmallVector<SymbolOccurrenceInfo, 8> Occurrences;
};

class Aggregator {
  std::string StorePath;

  BumpPtrAllocator Allocator;

  StringMap<FilePathIndex, BumpPtrAllocator &> FilePathIndices;
  std::vector<StringRef> FilePaths;
  StringMap<char, BumpPtrAllocator &> Triples;

  std::vector<std::unique_ptr<UnitInfo>> Units;

  StringMap<RecordIndex, BumpPtrAllocator &> RecordIndices;
  std::vector<std::unique_ptr<RecordInfo>> Records;

  StringMap<SymbolIndex, BumpPtrAllocator &> SymbolIndices;
  std::vector<SymbolInfo> Symbols;

public:
  explicit Aggregator(StringRef storePath)
  : StorePath(storePath),
    FilePathIndices(Allocator),
    Triples(Allocator),
    RecordIndices(Allocator),
    SymbolIndices(Allocator) {}

  void processUnit(IndexUnitReader &UnitReader);
  void dumpJSON(raw_ostream &OS);

private:
  StringRef copyStr(StringRef str) {
    if (str.empty())
      return StringRef();
    char *buf = Allocator.Allocate<char>(str.size());
    std::copy(str.begin(), str.end(), buf);
    return StringRef(buf, str.size());
  }

  StringRef getTripleString(StringRef inputTriple) {
    return Triples.insert(std::make_pair(inputTriple, 0)).first->first();
  }

  FilePathIndex getFilePathIndex(StringRef path, StringRef workingDir);
  RecordIndex getRecordIndex(StringRef recordFile);
  SymbolIndex getSymbolIndex(const IndexRecordDecl *dcl);
  std::unique_ptr<RecordInfo> processRecord(StringRef recordFile);
};

} // anonymous namespace

void Aggregator::processUnit(IndexUnitReader &UnitReader) {
  auto workDir = UnitReader.getWorkingDirectory();
  auto unit = llvm::make_unique<UnitInfo>();
  unit->Triple = getTripleString(UnitReader.getTarget());
  unit->OutFile = getFilePathIndex(UnitReader.getOutputFile(), workDir);

  auto depends = UnitReader.getDependencies();
  unit->Sources.reserve(depends.size());
  for (auto dependFilename : UnitReader.getDependencies()) {
    UnitSourceInfo source;
    source.FilePath = getFilePathIndex(dependFilename, workDir);
    unit->Sources.push_back(std::move(source));
  }
  UnitReader.foreachRecord([&](StringRef recordFile, StringRef filename, unsigned depIndex) -> bool {
    RecordIndex recIndex = getRecordIndex(recordFile);
    unit->Sources[depIndex].AssociatedRecords.push_back(recIndex);
    return true;
  });

  Units.push_back(std::move(unit));
}

FilePathIndex Aggregator::getFilePathIndex(StringRef path, StringRef workingDir) {
  StringRef absPath;
  SmallString<128> absPathBuf;
  if (sys::path::is_absolute(path) || workingDir.empty()) {
    absPath = path;
  } else {
    absPathBuf = workingDir;
    sys::path::append(absPathBuf, path);
    absPath = absPathBuf.str();
  }

  auto pair = FilePathIndices.insert(std::make_pair(absPath, FilePaths.size()));
  bool wasInserted = pair.second;
  if (wasInserted) {
    FilePaths.push_back(pair.first->first());
  }
  return pair.first->second;
}

RecordIndex Aggregator::getRecordIndex(StringRef recordFile) {
  auto pair = RecordIndices.insert(std::make_pair(recordFile, Records.size()));
  bool wasInserted = pair.second;
  if (wasInserted) {
    Records.push_back(processRecord(recordFile));
  }
  return pair.first->second;
}

std::unique_ptr<RecordInfo> Aggregator::processRecord(StringRef recordFile) {
  std::string error;
  auto recordReader = IndexRecordReader::createWithRecordFilename(recordFile, StorePath, error);
  if (!recordReader) {
    errs() << "failed reading record file: " << recordFile << '\n';
    ::exit(1);
  }
  auto record = llvm::make_unique<RecordInfo>();
  recordReader->foreachOccurrence([&](const IndexRecordOccurrence &idxOccur) -> bool {
    SymbolIndex symIdx = getSymbolIndex(idxOccur.Dcl);
    SymbolInfo &symInfo = Symbols[symIdx];
    symInfo.Roles |= idxOccur.Roles;
    SymbolOccurrenceInfo occurInfo;
    occurInfo.Symbol = symIdx;
    for (auto &rel : idxOccur.Relations) {
      SymbolIndex relsymIdx = getSymbolIndex(rel.Dcl);
      SymbolInfo &relsymInfo = Symbols[relsymIdx];
      relsymInfo.RelatedRoles |= rel.Roles;
      occurInfo.Relations.emplace_back(relsymIdx, rel.Roles);
    }
    occurInfo.Roles = idxOccur.Roles;
    occurInfo.Line = idxOccur.Line;
    occurInfo.Column = idxOccur.Column;
    record->Occurrences.push_back(std::move(occurInfo));
    return true;
  });
  return record;
}

SymbolIndex Aggregator::getSymbolIndex(const IndexRecordDecl *dcl) {
  auto pair = SymbolIndices.insert(std::make_pair(dcl->USR, Symbols.size()));
  bool wasInserted = pair.second;
  if (wasInserted) {
    SymbolInfo symInfo;
    symInfo.Kind = dcl->Kind;
    symInfo.Lang = dcl->Lang;
    symInfo.USR = pair.first->first();
    symInfo.Name = copyStr(dcl->Name);
    symInfo.CodegenName = copyStr(dcl->CodeGenName);
    Symbols.push_back(std::move(symInfo));
  }
  return pair.first->second;
}


void Aggregator::dumpJSON(raw_ostream &OS) {
  OS << "{\n";
  OS.indent(2) << "\"files\": [\n";
  for (unsigned i = 0, e = FilePaths.size(); i != e; ++i) {
    OS.indent(4) << '\"' << FilePaths[i] << '\"';
    if (i < e-1) OS << ',';
    OS << '\n';
  }
  OS.indent(2) << "],\n";

  OS.indent(2) << "\"symbols\": [\n";
  for (unsigned i = 0, e = Symbols.size(); i != e; ++i) {
    OS.indent(4) << "{\n";
    SymbolInfo &symInfo = Symbols[i];
    OS.indent(6) << "\"kind\": \"" << getSymbolKindString(symInfo.Kind) << "\",\n";
    OS.indent(6) << "\"lang\": \"" << getSymbolLanguageString(symInfo.Lang) << "\",\n";
    OS.indent(6) << "\"usr\": \"" << symInfo.USR << "\",\n";
    OS.indent(6) << "\"name\": \"" << symInfo.Name << "\",\n";
    if (!symInfo.CodegenName.empty())
      OS.indent(6) << "\"codegen\": \"" << symInfo.CodegenName << "\",\n";
    OS.indent(6) << "\"roles\": \"";
    printSymbolRoles(symInfo.Roles, OS);
    OS << '\"';
    if (symInfo.RelatedRoles != 0) {
      OS << ",\n";
      OS.indent(6) << "\"rel-roles\": \"";
      printSymbolRoles(symInfo.RelatedRoles, OS);
      OS << '\"';
    }
    OS << '\n';
    OS.indent(4) << "}";
    if (i < e-1) OS << ',';
    OS << '\n';
  }
  OS.indent(2) << "],\n";

  OS.indent(2) << "\"records\": [\n";
  for (unsigned i = 0, e = Records.size(); i != e; ++i) {
    OS.indent(4) << "{\n";
    RecordInfo &recInfo = *Records[i];
    OS.indent(6) << "\"occurrences\": [\n";
    for (unsigned oi = 0, oe = recInfo.Occurrences.size(); oi != oe; ++oi) {
      OS.indent(8) << "{\n";
      SymbolOccurrenceInfo &occurInfo = recInfo.Occurrences[oi];
      OS.indent(10) << "\"symbol\": " << occurInfo.Symbol << ",\n";
      OS.indent(10) << "\"line\": " << occurInfo.Line << ",\n";
      OS.indent(10) << "\"col\": " << occurInfo.Column << ",\n";
      OS.indent(10) << "\"roles\": \"";
      printSymbolRoles(occurInfo.Roles, OS);
      OS << '\"';
      if (!occurInfo.Relations.empty()) {
        OS << ",\n";
        OS.indent(10) << "\"relations\": [\n";
        for (unsigned ri = 0, re = occurInfo.Relations.size(); ri != re; ++ri) {
          OS.indent(12) << "{\n";
          SymbolRelationInfo &relInfo = occurInfo.Relations[ri];
          OS.indent(14) << "\"symbol\": " << relInfo.RelatedSymbol << ",\n";
          OS.indent(14) << "\"rel-roles\": \"";
          printSymbolRoles(relInfo.Roles, OS);
          OS << "\"\n";
          OS.indent(12) << "}";
          if (ri < re-1) OS << ',';
          OS << '\n';
        }
        OS.indent(10) << "]\n";
      }
      OS << '\n';
      OS.indent(8) << "}";
      if (oi < oe-1) OS << ',';
      OS << '\n';
    }
    OS.indent(6) << "]\n";
    OS.indent(4) << "}";
    if (i < e-1) OS << ',';
    OS << '\n';
  }
  OS.indent(2) << "],\n";

  OS.indent(2) << "\"units\": [\n";
  for (unsigned i = 0, e = Units.size(); i != e; ++i) {
    OS.indent(4) << "{\n";
    UnitInfo &unit = *Units[i];
    OS.indent(6) << "\"triple\": \"" << unit.Triple << "\",\n";
    OS.indent(6) << "\"out-file\": \"" << unit.OutFile << "\",\n";
    OS.indent(6) << "\"sources\": [\n";
    for (unsigned si = 0, se = unit.Sources.size(); si != se; ++si) {
      OS.indent(8) << "{\n";
      UnitSourceInfo &source = unit.Sources[si];
      OS.indent(10) << "\"file\": " << source.FilePath;
      if (!source.AssociatedRecords.empty()) {
        OS << ",\n";
        OS.indent(10) << "\"records\": [";
        for (unsigned ri = 0, re = source.AssociatedRecords.size(); ri != re; ++ri) {
          OS << source.AssociatedRecords[ri];
          if (ri < re-1) OS << ", ";
        }
        OS << ']';
      }
      OS << '\n';
      OS.indent(8) << "}";
      if (si < se-1) OS << ',';
      OS << '\n';
    }
    OS.indent(6) << "]\n";
    OS.indent(4) << "}";
    if (i < e-1) OS << ',';
    OS << '\n';
  }
  OS.indent(2) << "]\n";
  OS << "}\n";
}


bool index::aggregateDataAsJSON(StringRef StorePath, raw_ostream &OS) {
  std::string error;
  auto dataStore = IndexDataStore::create(StorePath, error);
  if (!dataStore) {
    errs() << "error opening store path '" << StorePath << "': " << error << '\n';
    return true;
  }

  auto unitPaths = dataStore->getAllUnitPaths();
  // Explicitely avoid doing any memory cleanup for aggregator since the process
  // is going to exit when we are done.
  Aggregator *aggregator = new Aggregator(StorePath);
  for (auto &unitPath : unitPaths) {
    auto unitReader = IndexUnitReader::create(unitPath, error);
    if (!unitReader) {
      errs() << "error opening unit file '" << unitPath << "': " << error << '\n';
      return true;
    }

    aggregator->processUnit(*unitReader);
  }

  aggregator->dumpJSON(OS);
  return false;
}
