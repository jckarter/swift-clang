//===--- APINotesMAnager.cpp - Manage API Notes Files ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file implements the APINotesManager class.
//
//===----------------------------------------------------------------------===//

#include "clang/APINotes/APINotesManager.h"
#include "clang/APINotes/APINotesReader.h"
#include "clang/APINotes/APINotesYAMLCompiler.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/Version.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Path.h"

using namespace clang;
using namespace api_notes;

#define DEBUG_TYPE "API Notes"
STATISTIC(NumHeaderAPINotes,
          "non-framework API notes files loaded");
STATISTIC(NumPublicFrameworkAPINotes,
          "framework public API notes loaded");
STATISTIC(NumPrivateFrameworkAPINotes,
          "framework private API notes loaded");
STATISTIC(NumFrameworksSearched,
          "frameworks searched");
STATISTIC(NumDirectoriesSearched,
          "header directories searched");
STATISTIC(NumDirectoryCacheHits,
          "directory cache hits");
STATISTIC(NumBinaryCacheHits,
          "binary form cache hits");
STATISTIC(NumBinaryCacheMisses,
          "binary form cache misses");
STATISTIC(NumBinaryCacheRebuilds,
          "binary form cache rebuilds");

APINotesManager::APINotesManager(SourceManager &SourceMgr)
  : SourceMgr(SourceMgr) { }


APINotesManager::~APINotesManager() {
  // Free the API notes readers.
  for (const auto &entry : Readers) {
    if (auto reader = entry.second.dyn_cast<APINotesReader *>()) {
      delete reader;
    }
  }
}


bool APINotesManager::loadAPINotes(const DirectoryEntry *HeaderDir,
                                   const FileEntry *APINotesFile) {
  assert(Readers.find(HeaderDir) == Readers.end());

  FileManager &FileMgr = SourceMgr.getFileManager();

  // If the API notes file is already in the binary form, load it directly.
  StringRef APINotesFileName = APINotesFile->getName();
  StringRef APINotesFileExt = llvm::sys::path::extension(APINotesFileName);
  if (!APINotesFileExt.empty() &&
      APINotesFileExt.substr(1) == BINARY_APINOTES_EXTENSION) {
    // Load the file.
    auto Buffer = FileMgr.getBufferForFile(APINotesFile);
    if (!Buffer) {
      Readers[HeaderDir] = nullptr;
      return true;
    }

    // Load the binary form.
    auto Reader = APINotesReader::get(std::move(Buffer));
    if (!Reader) {
      Readers[HeaderDir] = nullptr;
      return true;
    }

    // Record the reader.
    Readers[HeaderDir] = Reader.release();
    return false;
  }

  // Compute a hash of the API notes file's directory and the Clang version,
  // to be used as part of the filename for the cached binary copy.
  auto code = llvm::hash_value(StringRef(APINotesFile->getDir()->getName()));
  code = hash_combine(code, getClangFullRepositoryVersion());

  // Determine the file name for the cached binary form.
  SmallString<128> CompiledFileName;
  CompiledFileName += FileMgr.getFileSystemOptions().APINotesCachePath;
  assert(!CompiledFileName.empty() && "No API notes cache path provided?");
  llvm::sys::path::append(CompiledFileName,
    (llvm::Twine(llvm::sys::path::stem(APINotesFileName)) + "-"
     + llvm::APInt(64, code).toString(36, /*Signed=*/false) + "."
     + BINARY_APINOTES_EXTENSION));

  // Try to open the cached binary form.
  if (const FileEntry *CompiledFile = FileMgr.getFile(CompiledFileName,
                                                      /*openFile=*/true,
                                                      /*cacheFailure=*/false)) {
    // Load the file contents.
    if (auto Buffer = FileMgr.getBufferForFile(CompiledFile)) {
      // Make sure the file is up-to-date.
      if (CompiledFile->getModificationTime()
            >= APINotesFile->getModificationTime()) {
        // Load the file.
        if (auto Reader = APINotesReader::get(std::move(Buffer))) {
          // Success.
          ++NumBinaryCacheHits;
          Readers[HeaderDir] = Reader.release();
          return false;
        }
      }
    }

    // The cache entry was somehow broken; delete this one so we can build a
    // new one below.
    llvm::sys::fs::remove(CompiledFileName.str());
    ++NumBinaryCacheRebuilds;
  } else {
    ++NumBinaryCacheMisses;
  }

  // Open the source file.
  auto Buffer = FileMgr.getBufferForFile(APINotesFile);
  if (!Buffer) {
    Readers[HeaderDir] = nullptr;
    return true;
  }

  // Compile the API notes source into a buffer.
  // FIXME: Either propagate OSType through or, better yet, improve the binary
  // APINotes format to maintain complete availability information.
  llvm::SmallVector<char, 1024> APINotesBuffer;
  {
    llvm::raw_svector_ostream OS(APINotesBuffer);
    if (api_notes::compileAPINotes(Buffer->getBuffer(), OS,
                                   api_notes::OSType::Absent)) {
      Readers[HeaderDir] = nullptr;
      return true;
    }
    OS.flush();

    // Make a copy of the compiled form into the buffer.
    Buffer = llvm::MemoryBuffer::getMemBufferCopy(
               StringRef(APINotesBuffer.data(), APINotesBuffer.size()));
  }

  // Save the binary form into the cache. Perform this operation
  // atomically.
  SmallString<64> TemporaryBinaryFileName = CompiledFileName.str();
  TemporaryBinaryFileName.erase(
    TemporaryBinaryFileName.end()
      - llvm::sys::path::extension(TemporaryBinaryFileName).size(),
    TemporaryBinaryFileName.end());
  TemporaryBinaryFileName += "-%%%%%%.";
  TemporaryBinaryFileName += BINARY_APINOTES_EXTENSION;

  int TemporaryFD;
  llvm::sys::fs::create_directories(
    FileMgr.getFileSystemOptions().APINotesCachePath);
  if (!llvm::sys::fs::createUniqueFile(TemporaryBinaryFileName.str(),
                                       TemporaryFD, TemporaryBinaryFileName)) {
    // Write the contents of the buffer.
    bool hadError;
    {
      llvm::raw_fd_ostream Out(TemporaryFD, /*shouldClose=*/true);
      Out.write(Buffer->getBufferStart(), Buffer->getBufferSize());
      Out.flush();

      hadError = Out.has_error();
    }

    if (!hadError) {
      // Rename the temporary file to the actual compiled file.
      llvm::sys::fs::rename(TemporaryBinaryFileName.str(),
                            CompiledFileName.str());
    }
  }

  // Load the binary form we just compiled.
  auto Reader = APINotesReader::get(std::move(Buffer));
  assert(Reader && "Could not load the API notes we just generated?");

  // Record the reader.
  Readers[HeaderDir] = Reader.release();
  return false;
}

const DirectoryEntry *APINotesManager::loadFrameworkAPINotes(
                        llvm::StringRef FrameworkPath,
                        llvm::StringRef FrameworkName,
                        bool Public) {
  FileManager &FileMgr = SourceMgr.getFileManager();
  
  llvm::SmallString<128> Path;
  Path += FrameworkPath;
  unsigned FrameworkNameLength = Path.size();

  // Form the path to the APINotes file.
  llvm::sys::path::append(Path, "APINotes");
  if (Public)
    llvm::sys::path::append(Path,
                            (llvm::Twine(FrameworkName) + "."
                              + SOURCE_APINOTES_EXTENSION));
  else
    llvm::sys::path::append(Path,
                            (llvm::Twine(FrameworkName) + "_private."
                              + SOURCE_APINOTES_EXTENSION));

  // Try to open the APINotes file.
  const FileEntry *APINotesFile = FileMgr.getFile(Path);
  if (!APINotesFile)
    return nullptr;

  // Form the path to the corresponding header directory.
  Path.resize(FrameworkNameLength);
  if (Public)
    llvm::sys::path::append(Path, "Headers");
  else
    llvm::sys::path::append(Path, "PrivateHeaders");

  // Try to access the header directory.
  const DirectoryEntry *HeaderDir = FileMgr.getDirectory(Path);
  if (!HeaderDir)
    return nullptr;

  // Try to load the API notes.
  if (loadAPINotes(HeaderDir, APINotesFile))
    return nullptr;

  // Success: return the header directory.
  if (Public)
    ++NumPublicFrameworkAPINotes;
  else
    ++NumPrivateFrameworkAPINotes;
  return HeaderDir;
}

APINotesReader *APINotesManager::findAPINotes(SourceLocation Loc) {
  // API notes are associated with the expansion location. Retrieve the
  // file for this location.
  SourceLocation ExpansionLoc = SourceMgr.getExpansionLoc(Loc);
  FileID ID = SourceMgr.getFileID(ExpansionLoc);
  if (ID.isInvalid())
    return nullptr;
  const FileEntry *File = SourceMgr.getFileEntryForID(ID);
  if (!File)
    return nullptr;

  // Look for API notes in the directory corresponding to this file, or one of
  // its its parent directories.
  const DirectoryEntry *Dir = File->getDir();
  FileManager &FileMgr = SourceMgr.getFileManager();
  llvm::SmallVector<const DirectoryEntry *, 4> DirsVisited;
  APINotesReader *Result = nullptr;
  do {
    // Look for an API notes reader for this header search directory.
    auto Known = Readers.find(Dir);

    // If we already know the answer, chase it.
    if (Known != Readers.end()) {
      ++NumDirectoryCacheHits;

      // We've been redirected to another directory for answers. Follow it.
      if (auto OtherDir = Known->second.dyn_cast<const DirectoryEntry *>()) {
        DirsVisited.push_back(Dir);
        Dir = OtherDir;
        continue;
      }

      // We have the answer.
      Result = Known->second.dyn_cast<APINotesReader *>();
      break;
    }

    // Look for API notes corresponding to this directory.
    StringRef Path = Dir->getName();
    if (llvm::sys::path::extension(Path) == ".framework") {
      // If this is a framework directory, check whether there are API notes
      // in the APINotes subdirectory.
      auto FrameworkName = llvm::sys::path::stem(Path);
      ++NumFrameworksSearched;

      // Look for API notes for both the public and private headers.
      const DirectoryEntry *PublicDir
        = loadFrameworkAPINotes(Path, FrameworkName, /*Public=*/true);
      const DirectoryEntry *PrivateDir
        = loadFrameworkAPINotes(Path, FrameworkName, /*Public=*/false);

      if (PublicDir || PrivateDir) {
        // We found API notes: don't ever look past the framework directory.
        Readers[Dir] = nullptr;

        // Pretend we found the result in the public or private directory,
        // as appropriate. All headers should be in one of those two places,
        // but be defensive here.
        if (!DirsVisited.empty()) {
          if (DirsVisited.back() == PublicDir) {
            DirsVisited.pop_back();
            Dir = PublicDir;
          } else if (DirsVisited.back() == PrivateDir) {
            DirsVisited.pop_back();
            Dir = PrivateDir;
          }
        }

        // Grab the result.
        Result = Readers[Dir].dyn_cast<APINotesReader *>();;
        break;
      }
    } else {
      // Look for an APINotes file in this directory.
      llvm::SmallString<128> APINotesPath;
      APINotesPath += Dir->getName();
      llvm::sys::path::append(APINotesPath,
                              (llvm::Twine("APINotes.")
                                 + SOURCE_APINOTES_EXTENSION));

      // If there is an API notes file here, try to load it.
      ++NumDirectoriesSearched;
      if (const FileEntry *APINotesFile = FileMgr.getFile(APINotesPath)) {
        if (!loadAPINotes(Dir, APINotesFile)) {
          ++NumHeaderAPINotes;
          Result = Readers[Dir].dyn_cast<APINotesReader *>();
          break;
        }
      }
    }

    // We didn't find anything. Look at the parent directory.
    DirsVisited.push_back(Dir);
    StringRef ParentPath = llvm::sys::path::parent_path(Path);
    if (ParentPath.empty()) {
      Dir = nullptr;
    } else {
      Dir = FileMgr.getDirectory(ParentPath);
    }
  } while (Dir);

  // Path compression for all of the directories we visited, redirecting
  // them to the directory we ended on. If no API notes were found, the
  // resulting directory will be NULL, indicating no API notes.
  for (const auto Visited : DirsVisited) {
    Readers[Visited] = Dir;
  }

  return Result;
}
