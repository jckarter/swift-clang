//===--- TemplateBase.cpp - Common template AST class implementation ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements common classes used throughout C++ template
// representations.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/TemplateBase.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/TypeLoc.h"
#include "clang/Basic/Diagnostic.h"
#include "llvm/ADT/FoldingSet.h"
#include <algorithm>

using namespace clang;

//===----------------------------------------------------------------------===//
// TemplateArgument Implementation
//===----------------------------------------------------------------------===//

TemplateArgument TemplateArgument::CreatePackCopy(ASTContext &Context,
                                                  const TemplateArgument *Args,
                                                  unsigned NumArgs) {
  if (NumArgs == 0)
    return TemplateArgument(0, 0);
  
  TemplateArgument *Storage = new (Context) TemplateArgument [NumArgs];
  std::copy(Args, Args + NumArgs, Storage);
  return TemplateArgument(Storage, NumArgs);
}

bool TemplateArgument::isDependent() const {
  switch (getKind()) {
  case Null:
    assert(false && "Should not have a NULL template argument");
    return false;

  case Type:
    return getAsType()->isDependentType();

  case Template:
    return getAsTemplate().isDependent();

  case TemplateExpansion:
    return true;

  case Declaration:
    if (DeclContext *DC = dyn_cast<DeclContext>(getAsDecl()))
      return DC->isDependentContext();
    return getAsDecl()->getDeclContext()->isDependentContext();

  case Integral:
    // Never dependent
    return false;

  case Expression:
    return (getAsExpr()->isTypeDependent() || getAsExpr()->isValueDependent());

  case Pack:
    for (pack_iterator P = pack_begin(), PEnd = pack_end(); P != PEnd; ++P) {
      if (P->isDependent())
        return true;
    }

    return false;
  }

  return false;
}

bool TemplateArgument::isPackExpansion() const {
  switch (getKind()) {
  case Null:
  case Declaration:
  case Integral:
  case Pack:    
  case Template:
    return false;
      
  case TemplateExpansion:
    return true;
      
  case Type:
    return isa<PackExpansionType>(getAsType());
          
  case Expression:
    return isa<PackExpansionExpr>(getAsExpr());
  }
  
  return false;
}

bool TemplateArgument::containsUnexpandedParameterPack() const {
  switch (getKind()) {
  case Null:
  case Declaration:
  case Integral:
  case TemplateExpansion:
    break;

  case Type:
    if (getAsType()->containsUnexpandedParameterPack())
      return true;
    break;

  case Template:
    if (getAsTemplate().containsUnexpandedParameterPack())
      return true;
    break;
        
  case Expression:
    if (getAsExpr()->containsUnexpandedParameterPack())
      return true;
    break;

  case Pack:
    for (pack_iterator P = pack_begin(), PEnd = pack_end(); P != PEnd; ++P)
      if (P->containsUnexpandedParameterPack())
        return true;

    break;
  }

  return false;
}

void TemplateArgument::Profile(llvm::FoldingSetNodeID &ID,
                               const ASTContext &Context) const {
  ID.AddInteger(Kind);
  switch (Kind) {
  case Null:
    break;

  case Type:
    getAsType().Profile(ID);
    break;

  case Declaration:
    ID.AddPointer(getAsDecl()? getAsDecl()->getCanonicalDecl() : 0);
    break;

  case Template:
  case TemplateExpansion: {
    TemplateName Template = getAsTemplateOrTemplatePattern();
    if (TemplateTemplateParmDecl *TTP
          = dyn_cast_or_null<TemplateTemplateParmDecl>(
                                                Template.getAsTemplateDecl())) {
      ID.AddBoolean(true);
      ID.AddInteger(TTP->getDepth());
      ID.AddInteger(TTP->getPosition());
      ID.AddBoolean(TTP->isParameterPack());
    } else {
      ID.AddBoolean(false);
      ID.AddPointer(Context.getCanonicalTemplateName(Template)
                                                          .getAsVoidPointer());
    }
    break;
  }
      
  case Integral:
    getAsIntegral()->Profile(ID);
    getIntegralType().Profile(ID);
    break;

  case Expression:
    getAsExpr()->Profile(ID, Context, true);
    break;

  case Pack:
    ID.AddInteger(Args.NumArgs);
    for (unsigned I = 0; I != Args.NumArgs; ++I)
      Args.Args[I].Profile(ID, Context);
  }
}

bool TemplateArgument::structurallyEquals(const TemplateArgument &Other) const {
  if (getKind() != Other.getKind()) return false;

  switch (getKind()) {
  case Null:
  case Type:
  case Declaration:
  case Expression:      
  case Template:
  case TemplateExpansion:
    return TypeOrValue == Other.TypeOrValue;

  case Integral:
    return getIntegralType() == Other.getIntegralType() &&
           *getAsIntegral() == *Other.getAsIntegral();

  case Pack:
    if (Args.NumArgs != Other.Args.NumArgs) return false;
    for (unsigned I = 0, E = Args.NumArgs; I != E; ++I)
      if (!Args.Args[I].structurallyEquals(Other.Args.Args[I]))
        return false;
    return true;
  }

  // Suppress warnings.
  return false;
}

TemplateArgument TemplateArgument::getPackExpansionPattern() const {
  assert(isPackExpansion());
  
  switch (getKind()) {
  case Type:
    return getAsType()->getAs<PackExpansionType>()->getPattern();
    
  case Expression:
    return cast<PackExpansionExpr>(getAsExpr())->getPattern();
    
  case TemplateExpansion:
    return TemplateArgument(getAsTemplateOrTemplatePattern(), false);
    
  case Declaration:
  case Integral:
  case Pack:
  case Null:
  case Template:
    return TemplateArgument();
  }
  
  return TemplateArgument();
}

void TemplateArgument::print(const PrintingPolicy &Policy, 
                             llvm::raw_ostream &Out) const {
  switch (getKind()) {
  case Null:
    Out << "<no value>";
    break;
    
  case Type: {
    std::string TypeStr;
    getAsType().getAsStringInternal(TypeStr, Policy);
    Out << TypeStr;
    break;
  }
    
  case Declaration: {
    bool Unnamed = true;
    if (NamedDecl *ND = dyn_cast_or_null<NamedDecl>(getAsDecl())) {
      if (ND->getDeclName()) {
        Unnamed = false;
        Out << ND->getNameAsString();
      }
    }
    
    if (Unnamed) {
      Out << "<anonymous>";
    }
    break;
  }
    
  case Template:
    getAsTemplate().print(Out, Policy);
    break;

  case TemplateExpansion:
    getAsTemplateOrTemplatePattern().print(Out, Policy);
    Out << "...";
    break;
      
  case Integral: {
    Out << getAsIntegral()->toString(10);
    break;
  }
    
  case Expression:
    getAsExpr()->printPretty(Out, 0, Policy);
    break;
    
  case Pack:
    Out << "<";
    bool First = true;
    for (TemplateArgument::pack_iterator P = pack_begin(), PEnd = pack_end();
         P != PEnd; ++P) {
      if (First)
        First = false;
      else
        Out << ", ";
      
      P->print(Policy, Out);
    }
    Out << ">";
    break;        
  }
}

//===----------------------------------------------------------------------===//
// TemplateArgumentLoc Implementation
//===----------------------------------------------------------------------===//

TemplateArgumentLocInfo::TemplateArgumentLocInfo() {
  memset(this, 0, sizeof(TemplateArgumentLocInfo));
}

SourceRange TemplateArgumentLoc::getSourceRange() const {
  switch (Argument.getKind()) {
  case TemplateArgument::Expression:
    return getSourceExpression()->getSourceRange();

  case TemplateArgument::Declaration:
    return getSourceDeclExpression()->getSourceRange();

  case TemplateArgument::Type:
    if (TypeSourceInfo *TSI = getTypeSourceInfo())
      return TSI->getTypeLoc().getSourceRange();
    else
      return SourceRange();

  case TemplateArgument::Template:
    if (getTemplateQualifierRange().isValid())
      return SourceRange(getTemplateQualifierRange().getBegin(), 
                         getTemplateNameLoc());
    return SourceRange(getTemplateNameLoc());

  case TemplateArgument::TemplateExpansion:
    if (getTemplateQualifierRange().isValid())
      return SourceRange(getTemplateQualifierRange().getBegin(), 
                         getTemplateEllipsisLoc());
    return SourceRange(getTemplateNameLoc(), getTemplateEllipsisLoc());

  case TemplateArgument::Integral:
  case TemplateArgument::Pack:
  case TemplateArgument::Null:
    return SourceRange();
  }

  // Silence bonus gcc warning.
  return SourceRange();
}

TemplateArgumentLoc 
TemplateArgumentLoc::getPackExpansionPattern(SourceLocation &Ellipsis,
                                       llvm::Optional<unsigned> &NumExpansions,
                                             ASTContext &Context) const {
  assert(Argument.isPackExpansion());
  
  switch (Argument.getKind()) {
  case TemplateArgument::Type: {
    // FIXME: We shouldn't ever have to worry about missing
    // type-source info!
    TypeSourceInfo *ExpansionTSInfo = getTypeSourceInfo();
    if (!ExpansionTSInfo)
      ExpansionTSInfo = Context.getTrivialTypeSourceInfo(
                                                     getArgument().getAsType(),
                                                         Ellipsis);
    PackExpansionTypeLoc Expansion
      = cast<PackExpansionTypeLoc>(ExpansionTSInfo->getTypeLoc());
    Ellipsis = Expansion.getEllipsisLoc();
    
    TypeLoc Pattern = Expansion.getPatternLoc();
    NumExpansions = Expansion.getTypePtr()->getNumExpansions();
    
    // FIXME: This is horrible. We know where the source location data is for
    // the pattern, and we have the pattern's type, but we are forced to copy
    // them into an ASTContext because TypeSourceInfo bundles them together
    // and TemplateArgumentLoc traffics in TypeSourceInfo pointers.
    TypeSourceInfo *PatternTSInfo
      = Context.CreateTypeSourceInfo(Pattern.getType(),
                                     Pattern.getFullDataSize());
    memcpy(PatternTSInfo->getTypeLoc().getOpaqueData(), 
           Pattern.getOpaqueData(), Pattern.getFullDataSize());
    return TemplateArgumentLoc(TemplateArgument(Pattern.getType()),
                               PatternTSInfo);
  }
      
  case TemplateArgument::Expression: {
    PackExpansionExpr *Expansion
      = cast<PackExpansionExpr>(Argument.getAsExpr());
    Expr *Pattern = Expansion->getPattern();
    Ellipsis = Expansion->getEllipsisLoc();
    NumExpansions = Expansion->getNumExpansions();
    return TemplateArgumentLoc(Pattern, Pattern);
  }

  case TemplateArgument::TemplateExpansion:
    // FIXME: Variadic templates num expansions
    Ellipsis = getTemplateEllipsisLoc();
    return TemplateArgumentLoc(Argument.getPackExpansionPattern(),
                               getTemplateQualifierRange(),
                               getTemplateNameLoc());
    
  case TemplateArgument::Declaration:
  case TemplateArgument::Template:
  case TemplateArgument::Integral:
  case TemplateArgument::Pack:
  case TemplateArgument::Null:
    return TemplateArgumentLoc();
  }
  
  return TemplateArgumentLoc();
}

const DiagnosticBuilder &clang::operator<<(const DiagnosticBuilder &DB,
                                           const TemplateArgument &Arg) {
  switch (Arg.getKind()) {
  case TemplateArgument::Null:
    // This is bad, but not as bad as crashing because of argument
    // count mismatches.
    return DB << "(null template argument)";
      
  case TemplateArgument::Type:
    return DB << Arg.getAsType();
      
  case TemplateArgument::Declaration:
    return DB << Arg.getAsDecl();
      
  case TemplateArgument::Integral:
    return DB << Arg.getAsIntegral()->toString(10);
      
  case TemplateArgument::Template:
    return DB << Arg.getAsTemplate();

  case TemplateArgument::TemplateExpansion:
    return DB << Arg.getAsTemplateOrTemplatePattern() << "...";

  case TemplateArgument::Expression: {
    // This shouldn't actually ever happen, so it's okay that we're
    // regurgitating an expression here.
    // FIXME: We're guessing at LangOptions!
    llvm::SmallString<32> Str;
    llvm::raw_svector_ostream OS(Str);
    LangOptions LangOpts;
    LangOpts.CPlusPlus = true;
    PrintingPolicy Policy(LangOpts);
    Arg.getAsExpr()->printPretty(OS, 0, Policy);
    return DB << OS.str();
  }
      
  case TemplateArgument::Pack: {
    // FIXME: We're guessing at LangOptions!
    llvm::SmallString<32> Str;
    llvm::raw_svector_ostream OS(Str);
    LangOptions LangOpts;
    LangOpts.CPlusPlus = true;
    PrintingPolicy Policy(LangOpts);
    Arg.print(Policy, OS);
    return DB << OS.str();
  }
  }
  
  return DB;
}
