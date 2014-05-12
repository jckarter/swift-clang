// RUN: %clang_cc1  -triple x86_64-apple-darwin10 -fsyntax-only -verify %s
// rdar://16076729

@partial_interface MySwiftClass;  // expected-note {{'MySwiftClass' declared here}}

@partial_interface MySwiftClassDup; // expected-note {{previous declaration is here}}

@partial_interface MySwiftClassDup; // expected-error {{duplicate partial class declaration for class 'MySwiftClassDup'}}


@interface DeclaredAsObjCClass @end // expected-note {{previous definition is here}}

@partial_interface DeclaredAsObjCClass; // expected-error {{duplicate partial class declaration for class 'DeclaredAsObjCClass'}}

typedef int SwiftClassDeclaredAsTypedefInt; // expected-note {{previous definition is here}}

@partial_interface SwiftClassDeclaredAsTypedefInt; // expected-error {{redefinition of 'SwiftClassDeclaredAsTypedefInt' as different kind of symbol}}


@partial_interface MySwiftClassMustMatchWithSwiftClass; // expected-note {{previous definition is here}}

@interface MySwiftClassMustMatchWithSwiftClass @end; // expected-error {{attempting to define an ordinary class for partial class 'MySwiftClassMustMatchWithSwiftClass'}}

@class ForwardSwiftClass;
@partial_interface ForwardSwiftClass; // expected-note {{previous declaration is here}}
@class ForwardSwiftClass;
@partial_interface ForwardSwiftClass; // expected-error {{duplicate partial class declaration for class 'ForwardSwiftClass'}}

@partial_interface SwiftClassWithNoDefSuperSwiftClass : MySwiftSuperClass; // expected-error {{cannot find interface declaration for 'MySwiftSuperClass', superclass of 'SwiftClassWithNoDefSuperSwiftClass'}}

typedef int BogusSuperClass; // expected-note {{previous definition is here}}
@partial_interface SwiftClassWithSymbol : BogusSuperClass; // expected-error {{redefinition of 'BogusSuperClass' as different kind of symbol}}

@partial_interface SwiftRootClass;
@partial_interface ValidSwiftClassWithRoot : SwiftRootClass; // Ok. Root is another partial class.

@partial_interface ValidSwiftClassWithObjCRoot : DeclaredAsObjCClass; // Ok. Root is ObjC class

@class MyAttemptToUseForwardClass; // expected-note {{forward declaration of class here}}

@partial_interface SwiftClassUsingForwardSuper : MyAttemptToUseForwardClass; // expected-error {{attempting to use the forward class 'MyAttemptToUseForwardClass' as superclass of 'SwiftClassUsingForwardSuper'}}

@interface ObjCRootClass @end

@partial_interface MySwiftRootClass : ObjCRootClass; // expected-note {{previous definition is here}}

@interface ObjCClass : MySwiftRootClass
@end

@partial_interface MySwift2ndClass : ObjCClass;

@partial_interface MySwiftSubClass : MySwift2ndClass;

@interface ObjCSubClass : MySwiftSubClass
@end

@interface MySwiftRootClass // expected-error {{attempting to define an ordinary class for partial class 'MySwiftRootClass'}}
@end

__attribute((objc_complete_definition))
@interface MySwiftSubClass : MySwift2ndClass
@end

@partial_interface foo; // expected-note {{class is declared here}}
@class foo;
@implementation foo // expected-error {{attempting to implement  partial class foo}}
@end
