// RUN: %clang_cc1  -triple x86_64-apple-darwin10 -fsyntax-only -verify %s
// rdar://16076729

@partial_interface BaseClass;

__attribute((objc_complete_definition))
@interface BaseClass @end


@interface xx : BaseClass;
@end

@interface xx(CAT)
@end

@implementation xx @end // ok!

@implementation xx(CAT)
@end

@interface yy : xx
@end

@interface yy(CAT)
@end

@implementation yy @end // ok!

@implementation yy(CAT)
@end

@interface zz : yy
@end

@implementation zz @end // ok!


__attribute((objc_complete_definition))
@interface PartialClass @end

@partial_interface PartialClass;

@interface aa : PartialClass @end

@interface bb : aa @end

@interface cc : bb @end

@interface aa(CAT) @end

@interface bb(CAT) @end

@interface cc(CAT) @end

@implementation aa @end // ok!
@implementation bb @end // ok!
@implementation cc @end // ok!

@implementation aa(CAT) @end // ok!
@implementation bb(CAT) @end // ok!
@implementation cc(CAT) @end // ok!


@partial_interface SuperPartialClass : PartialClass; // expected-note {{previous definition is here}}

@partial_interface DuperPartialClass : SuperPartialClass; // expected-note {{previous definition is here}}

__attribute((objc_complete_definition))
@interface DuperPartialClass @end // expected-error {{partial class and its complete definition must have same super class}}

__attribute((objc_complete_definition))
@interface SuperPartialClass @end // expected-error {{partial class and its complete definition must have same super class}}

@interface ObjCClass : DuperPartialClass 
@end

@implementation ObjCClass @end

@partial_interface PartialNoDefClass; // expected-error 3 {{attempting to use partial class 'PartialNoDefClass' as base class in an implementation}}

@interface dd : aa @end

@interface ee : PartialNoDefClass @end

@interface ff : ee @end

@interface ff(CAT)
@end

@interface dd(CAT)
@end

@implementation dd @end   // ok!

@implementation dd(CAT) @end   // ok!

@implementation ee @end // expected-note {{class implementation is declared here}}

@implementation ff @end // expected-note {{class implementation is declared here}}

@implementation ff(CAT) @end // expected-note {{category implementation is declared here}}

@partial_interface gg; // expected-note {{declared here}}

@interface gg(CAT)
@end

@interface gg(CAT2)
@end

@implementation gg(CAT) // expected-error {{attempting to implemenent category of partial class gg}}
@end

__attribute((objc_complete_definition))
@interface gg
@end

@implementation gg(CAT2)
@end
