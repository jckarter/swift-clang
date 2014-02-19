// RUN: %clang_cc1  -triple x86_64-apple-darwin10 -fsyntax-only -verify %s
// rdar://16076729

@partial_interface MyShinyClass;  // expected-note {{'MyShinyClass' declared here}}

@partial_interface MyShinyClassDup; // expected-note {{previous declaration is here}}

@partial_interface MyShinyClassDup; // expected-error {{duplicate partial class declaration for class 'MyShinyClassDup'}}


@interface DeclaredAsObjCClass @end // expected-note {{previous definition is here}}

@partial_interface DeclaredAsObjCClass; // expected-error {{duplicate partial class declaration for class 'DeclaredAsObjCClass'}}

typedef int ShinyClassDeclaredAsTypedefInt; // expected-note {{previous definition is here}}

@partial_interface ShinyClassDeclaredAsTypedefInt; // expected-error {{redefinition of 'ShinyClassDeclaredAsTypedefInt' as different kind of symbol}}


@partial_interface MyShinyClassMustMatchWithShinyClass; // expected-note {{previous definition is here}}

@interface MyShinyClassMustMatchWithShinyClass @end; // expected-error {{attempting to define an ordinary class for partial class 'MyShinyClassMustMatchWithShinyClass'}}

@class ForwardShinyClass;
@partial_interface ForwardShinyClass; // expected-note {{previous declaration is here}}
@class ForwardShinyClass;
@partial_interface ForwardShinyClass; // expected-error {{duplicate partial class declaration for class 'ForwardShinyClass'}}

@partial_interface ShinyClassWithNoDefSuperShinyClass : MyShinySuperClass; // expected-error {{cannot find interface declaration for 'MyShinySuperClass', superclass of 'ShinyClassWithNoDefSuperShinyClass'}}

typedef int BogusSuperClass; // expected-note {{previous definition is here}}
@partial_interface ShinyClassWithSymbol : BogusSuperClass; // expected-error {{redefinition of 'BogusSuperClass' as different kind of symbol}}

@partial_interface ShinyRootClass;
@partial_interface ValidShinyClassWithRoot : ShinyRootClass; // Ok. Root is another partial class.

@partial_interface ValidShinyClassWithObjCRoot : DeclaredAsObjCClass; // Ok. Root is ObjC class

@class MyAttemptToUseForwardClass; // expected-note {{forward declaration of class here}}

@partial_interface ShinyClassUsingForwardSuper : MyAttemptToUseForwardClass; // expected-error {{attempting to use the forward class 'MyAttemptToUseForwardClass' as superclass of 'ShinyClassUsingForwardSuper'}}

@interface ObjCRootClass @end

@partial_interface MyShinyRootClass : ObjCRootClass; // expected-note {{previous definition is here}}

@interface ObjCClass : MyShinyRootClass
@end

@partial_interface MyShiny2ndClass : ObjCClass;

@partial_interface MyShinySubClass : MyShiny2ndClass;

@interface ObjCSubClass : MyShinySubClass
@end

@interface MyShinyRootClass // expected-error {{attempting to define an ordinary class for partial class 'MyShinyRootClass'}}
@end

__attribute((objc_complete_definition))
@interface MyShinySubClass : MyShiny2ndClass
@end
