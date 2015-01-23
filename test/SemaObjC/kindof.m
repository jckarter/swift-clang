// RUN: %clang_cc1 -fsyntax-only %s -verify

// Tests Objective-C 'kindof' types.

@protocol NSObject
@end

@protocol NSCopying
@end

__attribute__((objc_root_class))
@interface NSObject <NSObject>
@end

@interface NSString : NSObject <NSCopying>
@end

@interface NSMutableString : NSString
@end

@interface NSNumber : NSObject <NSCopying>
@end

// ---------------------------------------------------------------------------
// Parsing and semantic analysis for __kindof
// ---------------------------------------------------------------------------

// Test proper application of __kindof.
typedef __kindof NSObject *typedef1;
typedef NSObject __kindof *typedef2;
typedef __kindof NSObject<NSCopying> typedef3;
typedef NSObject<NSCopying> __kindof *typedef4;
typedef __kindof id<NSCopying> typedef5;
typedef __kindof Class<NSCopying> typedef6;

// Test redundancy of __kindof.
typedef __kindof id __kindof redundant_typedef1;
typedef __kindof NSObject __kindof *redundant_typedef2;

// Test application of __kindof to typedefs.
typedef NSObject *NSObject_ptr_typedef;
typedef NSObject NSObject_typedef;
typedef __kindof NSObject_ptr_typedef typedef_typedef1;
typedef __kindof NSObject_typedef typedef_typedef2;

// Test application of __kindof to non-object types.
typedef __kindof int nonobject_typedef1; // expected-error{{'__kindof' specifier cannot be applied to non-object type 'int'}}
typedef NSObject **NSObject_ptr_ptr;
typedef __kindof NSObject_ptr_ptr nonobject_typedef2; // expected-error{{'__kindof' specifier cannot be applied to non-object type 'NSObject_ptr_ptr' (aka 'NSObject **')}}

// Test application of __kindof outside of the decl-specifiers.
typedef NSObject * __kindof bad_specifier_location1; // expected-error{{'__kindof' type specifier must precede the declarator}}
typedef NSObject bad_specifier_location2 __kindof; // expected-error{{expected ';' after top level declarator}}
// expected-warning@-1{{declaration does not declare anything}}

// ---------------------------------------------------------------------------
// Pretty printing of __kindof
// ---------------------------------------------------------------------------
void test_pretty_print(int *ip) {
  __kindof NSObject *kindof_NSObject;
  ip = kindof_NSObject; // expected-warning{{from '__kindof NSObject *'}}
 
  __kindof NSObject_ptr_typedef kindof_NSObject_ptr;
  ip = kindof_NSObject_ptr; // expected-warning{{from '__kindof NSObject_ptr_typedef'}}

  __kindof id <NSCopying> *kindof_NSCopying;
  ip = kindof_NSCopying; // expected-warning{{from '__kindof id<NSCopying> *'}}

  __kindof NSObject_ptr_typedef *kindof_NSObject_ptr_typedef;
  ip = kindof_NSObject_ptr_typedef; // expected-warning{{from '__kindof NSObject_ptr_typedef *'}}
}

// ---------------------------------------------------------------------------
// Basic implicit conversions (dropping __kindof, upcasts, etc.)
// ---------------------------------------------------------------------------
void test_add_remove_kindof_conversions(void) {
  __kindof NSObject *kindof_NSObject_obj;
  NSObject *NSObject_obj;
  __kindof NSString *kindof_NSString_obj;
  NSString *NSString_obj;

  // Conversion back and forth
  kindof_NSObject_obj = NSObject_obj;
  NSObject_obj = kindof_NSObject_obj;

  // Qualified-id conversion back and forth.
  __kindof id <NSCopying> kindof_id_NSCopying_obj;
  id <NSCopying> id_NSCopying_obj;
  kindof_id_NSCopying_obj = id_NSCopying_obj;
  id_NSCopying_obj = kindof_id_NSCopying_obj;
}

void test_upcast_conversions(void) {
  __kindof NSObject *kindof_NSObject_obj;
  NSObject *NSObject_obj;

  // Upcasts
  __kindof NSString *kindof_NSString_obj;
  NSString *NSString_obj;
  kindof_NSObject_obj = kindof_NSString_obj;
  kindof_NSObject_obj = NSString_obj;
  NSObject_obj = kindof_NSString_obj;
  NSObject_obj = NSString_obj;

  // "Upcasts" with qualified-id.
  __kindof id <NSCopying> kindof_id_NSCopying_obj;
  id <NSCopying> id_NSCopying_obj;
  kindof_id_NSCopying_obj = kindof_NSString_obj;
  kindof_id_NSCopying_obj = NSString_obj;
  id_NSCopying_obj = kindof_NSString_obj;
  id_NSCopying_obj = NSString_obj;
}


void test_ptr_object_conversions(void) {
  __kindof NSObject **ptr_kindof_NSObject_obj;
  NSObject **ptr_NSObject_obj;

  // Conversions back and forth.
  ptr_kindof_NSObject_obj = ptr_NSObject_obj;
  ptr_NSObject_obj = ptr_kindof_NSObject_obj;

  // Conversions back and forth with qualified-id.
  __kindof id <NSCopying> *ptr_kindof_id_NSCopying_obj;
  id <NSCopying> *ptr_id_NSCopying_obj;
  ptr_kindof_id_NSCopying_obj = ptr_id_NSCopying_obj;
  ptr_id_NSCopying_obj = ptr_kindof_id_NSCopying_obj;

  // Upcasts.
  __kindof NSString **ptr_kindof_NSString_obj;
  NSString **ptr_NSString_obj;
  ptr_kindof_NSObject_obj = ptr_kindof_NSString_obj;
  ptr_kindof_NSObject_obj = ptr_NSString_obj;
  ptr_NSObject_obj = ptr_kindof_NSString_obj;
  ptr_NSObject_obj = ptr_NSString_obj;
}
