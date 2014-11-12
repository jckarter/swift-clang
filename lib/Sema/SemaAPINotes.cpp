//===--- SemaAPINotes.cpp - API Notes Handling ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file implements the mapping from API notes to declaration attributes.
//
//===----------------------------------------------------------------------===//

#include "clang/Sema/SemaInternal.h"
#include "clang/AST/DeclObjC.h"
#include "clang/APINotes/APINotesReader.h"
using namespace clang;

static void ProcessAPINotes(Sema &S, Decl *D,
                            const api_notes::CommonEntityInfo &Info) {
  // Availability
  if (Info.Unavailable && !D->hasAttr<UnavailableAttr>()) {
    D->addAttr(UnavailableAttr::CreateImplicit(S.Context, Info.UnavailableMsg));
  }
}

/// Process API notes for a variable or property.
static void ProcessAPINotes(Sema &S, Decl *D,
                            const api_notes::VariableInfo &Info) {
  // Nullability.
  if (auto Nullability = Info.getNullability()) {
    if (*Nullability == Nullability::NonNull &&
        !D->hasAttr<NonNullAttr>()) {
      D->addAttr(NonNullAttr::CreateImplicit(S.Context, 0, 0));
    }
  }

  // Handle common entity information.
  ProcessAPINotes(S, D, static_cast<const api_notes::CommonEntityInfo &>(Info));
}

/// Process API notes for a global variable.
static void ProcessAPINotes(Sema &S, VarDecl *D,
                            const api_notes::GlobalVariableInfo &Info) {

  // Handle common entity information.
  ProcessAPINotes(S, D, static_cast<const api_notes::VariableInfo &>(Info));
}

/// Process API notes for an Objective-C property.
static void ProcessAPINotes(Sema &S, ObjCPropertyDecl *D,
                            const api_notes::ObjCPropertyInfo &Info) {

  // Handle common entity information.
  ProcessAPINotes(S, D, static_cast<const api_notes::VariableInfo &>(Info));
}

namespace {
  typedef llvm::PointerUnion<FunctionDecl *, ObjCMethodDecl *> FunctionOrMethod;
}

/// Process API notes for a function or method.
static void ProcessAPINotes(Sema &S, FunctionOrMethod AnyFunc,
                            const api_notes::FunctionInfo &Info) {
  // Find the declaration itself.
  FunctionDecl *FD = AnyFunc.dyn_cast<FunctionDecl *>();
  Decl *D = FD;
  ObjCMethodDecl *MD = 0;
  if (!D) {
    MD = AnyFunc.get<ObjCMethodDecl *>();
    D = MD;
  }

  // Nullability.
  if (Info.NullabilityAudited) {
    // Return type.
    if (Info.getReturnTypeInfo() == Nullability::NonNull &&
        !D->hasAttr<ReturnsNonNullAttr>()) {
      D->addAttr(ReturnsNonNullAttr::CreateImplicit(S.Context));
    }

    // Parameters.
    unsigned NumParams;
    if (FD)
      NumParams = FD->getNumParams();
    else
      NumParams = MD->param_size();

    for (unsigned I = 0; I != NumParams; ++I) {
      ParmVarDecl *Param;
      if (FD)
        Param = FD->getParamDecl(I);
      else
        Param = MD->param_begin()[I];

      if (Info.getParamTypeInfo(I) == Nullability::NonNull &&
          !Param->hasAttr<NonNullAttr>()) {
        Param->addAttr(NonNullAttr::CreateImplicit(S.Context, 0, 0));
      }
    }
  }

  // Handle common entity information.
  ProcessAPINotes(S, D, static_cast<const api_notes::CommonEntityInfo &>(Info));
}

/// Process API notes for a global function.
static void ProcessAPINotes(Sema &S, FunctionDecl *D,
                            const api_notes::GlobalFunctionInfo &Info) {

  // Handle common function information.
  ProcessAPINotes(S, FunctionOrMethod(D),
                  static_cast<const api_notes::FunctionInfo &>(Info));
}

/// Process API notes for an Objective-C method.
static void ProcessAPINotes(Sema &S, ObjCMethodDecl *D,
                            const api_notes::ObjCMethodInfo &Info) {
  // Designated initializers.
  if (Info.DesignatedInit && !D->getAttr<ObjCDesignatedInitializerAttr>()) {
    if (ObjCInterfaceDecl *IFace = D->getClassInterface()) {
      D->addAttr(ObjCDesignatedInitializerAttr::CreateImplicit(S.Context));
      IFace->setHasDesignatedInitializers();
    }
  }

  // Handle common function information.
  ProcessAPINotes(S, FunctionOrMethod(D),
                  static_cast<const api_notes::FunctionInfo &>(Info));
}

/// Process API notes for an Objective-C class or protocol.
static void ProcessAPINotes(Sema &S, ObjCContainerDecl *D,
                            const api_notes::ObjCContextInfo &Info) {

  // Handle common entity information.
  ProcessAPINotes(S, D, static_cast<const api_notes::CommonEntityInfo &>(Info));
}

/// Process API notes that are associated with this declaration, mapping them
/// to attributes as appropriate.
void Sema::ProcessAPINotes(Decl *D) {
  if (!Context.getLangOpts().APINotes)
    return;
  
  if (!D || D->getLocation().isInvalid())
    return;

  // Globals.
  if (D->getDeclContext()->isFileContext()) {
    // Global variables.
    if (auto VD = dyn_cast<VarDecl>(D)) {
      if (api_notes::APINotesReader *Reader
            = APINotes.findAPINotes(D->getLocation())) {
        if (auto Info = Reader->lookupGlobalVariable(VD->getName())) {
          ::ProcessAPINotes(*this, VD, *Info);
        }
      }

      return;
    }

    // Global functions.
    if (auto FD = dyn_cast<FunctionDecl>(D)) {
      if (FD->getDeclName().isIdentifier()) {
        if (api_notes::APINotesReader *Reader
              = APINotes.findAPINotes(D->getLocation())) {
          if (auto Info = Reader->lookupGlobalFunction(FD->getName())) {
            ::ProcessAPINotes(*this, FD, *Info);
          }
        }
      }

      return;
    }

    // Objective-C classes.
    if (auto Class = dyn_cast<ObjCInterfaceDecl>(D)) {
      if (api_notes::APINotesReader *Reader
            = APINotes.findAPINotes(D->getLocation())) {
        if (auto Info = Reader->lookupObjCClass(Class->getName())) {
          ::ProcessAPINotes(*this, Class, Info->second);
        }
      }

      return;
    }

    // Objective-C protocols.
    if (auto Protocol = dyn_cast<ObjCProtocolDecl>(D)) {
      if (api_notes::APINotesReader *Reader
            = APINotes.findAPINotes(D->getLocation())) {
        if (auto Info = Reader->lookupObjCProtocol(Protocol->getName())) {
          ::ProcessAPINotes(*this, Protocol, Info->second);
        }
      }

      return;
    }

    return;
  }

  if (auto ObjCContainer = dyn_cast<ObjCContainerDecl>(D->getDeclContext())) {
    // Location function that looks up an Objective-C context.
    auto GetContext = [&](api_notes::APINotesReader *Reader)
                        -> Optional<api_notes::ContextID> {
      if (auto Protocol = dyn_cast<ObjCProtocolDecl>(ObjCContainer)) {
        if (auto Found = Reader->lookupObjCProtocol(Protocol->getName()))
          return Found->first;

        return None;
      }

      if (auto Impl = dyn_cast<ObjCCategoryImplDecl>(ObjCContainer)) {
        if (auto Cat = Impl->getCategoryDecl())
          ObjCContainer = Cat;
        else
          return None;
      }

      if (auto Category = dyn_cast<ObjCCategoryDecl>(ObjCContainer)) {
        if (Category->getClassInterface())
          ObjCContainer = Category->getClassInterface();
        else
          return None;
      }

      if (auto Impl = dyn_cast<ObjCImplDecl>(ObjCContainer)) {
        if (Impl->getClassInterface())
          ObjCContainer = Impl->getClassInterface();
        else
          return None;
      }

      if (auto Class = dyn_cast<ObjCInterfaceDecl>(ObjCContainer)) {
        if (auto Found = Reader->lookupObjCClass(Class->getName()))
          return Found->first;

        return None;

      }

      return None;
    };

    // Objective-C methods.
    if (auto Method = dyn_cast<ObjCMethodDecl>(D)) {
      if (api_notes::APINotesReader *Reader
            = APINotes.findAPINotes(D->getLocation())) {
        if (auto Context = GetContext(Reader)) {
          // Map the selector.
          Selector Sel = Method->getSelector();
          SmallVector<StringRef, 2> SelPieces;
          if (Sel.isUnarySelector())
            SelPieces.push_back(Sel.getNameForSlot(0));
          else {
            for (unsigned i = 0, n = Sel.getNumArgs(); i != n; ++i)
              SelPieces.push_back(Sel.getNameForSlot(i));
          }

          api_notes::ObjCSelectorRef SelectorRef;
          SelectorRef.NumPieces = Sel.getNumArgs();
          SelectorRef.Identifiers = SelPieces;

          if (auto Info = Reader->lookupObjCMethod(*Context, SelectorRef,
                                                   Method->isInstanceMethod())){
            ::ProcessAPINotes(*this, Method, *Info);
          }
        }
      }
    }

    // Objective-C properties.
    if (auto Property = dyn_cast<ObjCPropertyDecl>(D)) {
      if (api_notes::APINotesReader *Reader
            = APINotes.findAPINotes(D->getLocation())) {
        if (auto Context = GetContext(Reader)) {
          if (auto Info = Reader->lookupObjCProperty(*Context,
                                                     Property->getName())) {
            ::ProcessAPINotes(*this, Property, *Info);
          }
        }
      }

      return;
    }

    return;
  }
}
