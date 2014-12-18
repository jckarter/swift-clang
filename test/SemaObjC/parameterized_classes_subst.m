// RUN: %clang_cc1 -fblocks -fsyntax-only %s -verify
//
// Test the substitution of type arguments for type parameters when
// using parameterized classes in Objective-C.

__attribute__((objc_root_class))
@interface NSObject
+ (instancetype)alloc;
- (instancetype)init;
@end

@protocol NSCopying
@end

@interface NSString : NSObject <NSCopying>
@end

@interface NSArray<T> : NSObject <NSCopying>
@end

@interface NSSet<T : id<NSCopying>> : NSObject <NSCopying>
- (T)firstObject;
@property (nonatomic, copy) NSArray<T> *allObjects;
@end

// Parameterized inheritance (simple case)
@interface NSMutableSet<U : id<NSCopying>> : NSSet<U>
- (void)addObject:(U)object; // expected-note 7{{passing argument to parameter 'object' here}}
@end

@interface Widget : NSObject <NSCopying>
@end

// Non-parameterized class inheriting from a specialization of a
// parameterized class.
@interface WidgetSet : NSMutableSet<Widget *>
@end

// Parameterized inheritance with a more interesting transformation in
// the specialization.
@interface MutableSetOfArrays<T> : NSMutableSet<NSArray<T>*>
@end

// Inheriting from an unspecialized form of a parameterized type.
@interface UntypedMutableSet : NSMutableSet
@end

@interface Window : NSObject
@end

// --------------------------------------------------------------------------
// Message sends.
// --------------------------------------------------------------------------
void test_message_send_result(
       NSSet<NSString *> *stringSet,
       NSMutableSet<NSString *> *mutStringSet,
       WidgetSet *widgetSet,
       UntypedMutableSet *untypedMutSet,
       MutableSetOfArrays<NSString *> *mutStringArraySet,
       NSSet *set,
       NSMutableSet *mutSet,
       MutableSetOfArrays *mutArraySet,
       void (^block)(void)) {
  int *ip;
  ip = [stringSet firstObject]; // expected-warning{{from 'NSString *'}}
  ip = [mutStringSet firstObject]; // expected-warning{{from 'NSString *'}}
  ip = [widgetSet firstObject]; // expected-warning{{from 'Widget *'}}
  ip = [untypedMutSet firstObject]; // expected-warning{{from 'id<NSCopying>'}}
  ip = [mutStringArraySet firstObject]; // expected-warning{{from 'NSArray<NSString *> *'}}
  ip = [set firstObject]; // expected-warning{{from 'id<NSCopying>'}}
  ip = [mutSet firstObject]; // expected-warning{{from 'id<NSCopying>'}}
  ip = [mutArraySet firstObject]; // expected-warning{{from 'NSArray<id> *'}}
  ip = [block firstObject]; // expected-warning{{from 'id<NSCopying>'}}
}

void test_message_send_param(
       NSMutableSet<NSString *> *mutStringSet,
       WidgetSet *widgetSet,
       UntypedMutableSet *untypedMutSet,
       MutableSetOfArrays<NSString *> *mutStringArraySet,
       NSMutableSet *mutSet,
       MutableSetOfArrays *mutArraySet,
       void (^block)(void)) {
  Window *window;

  [mutStringSet addObject: window]; // expected-warning{{parameter of type 'NSString *'}}
  [widgetSet addObject: window]; // expected-warning{{parameter of type 'Widget *'}}
  [untypedMutSet addObject: window]; // expected-warning{{parameter of incompatible type 'id<NSCopying>'}}
  [mutStringArraySet addObject: window]; // expected-warning{{parameter of type 'NSArray<NSString *> *'}}
  [mutSet addObject: window]; // expected-warning{{parameter of incompatible type 'id<NSCopying>'}}
  [mutArraySet addObject: window]; // expected-warning{{parameter of type 'NSArray<id> *'}}
  [block addObject: window]; // expected-warning{{parameter of incompatible type 'id<NSCopying>'}}
}

// --------------------------------------------------------------------------
// Property accesses.
// --------------------------------------------------------------------------
void test_property_read(
       NSSet<NSString *> *stringSet,
       NSMutableSet<NSString *> *mutStringSet,
       WidgetSet *widgetSet,
       UntypedMutableSet *untypedMutSet,
       MutableSetOfArrays<NSString *> *mutStringArraySet,
       NSSet *set,
       NSMutableSet *mutSet,
       MutableSetOfArrays *mutArraySet) {
  int *ip;
  ip = stringSet.allObjects; // expected-warning{{from 'NSArray<NSString *> *'}}
  ip = mutStringSet.allObjects; // expected-warning{{from 'NSArray<NSString *> *'}}
  ip = widgetSet.allObjects; // expected-warning{{from 'NSArray<Widget *> *'}}
  ip = untypedMutSet.allObjects; // expected-warning{{from 'NSArray<id<NSCopying>> *'}}
  ip = mutStringArraySet.allObjects; // expected-warning{{from 'NSArray<NSArray<NSString *> *> *'}}
  ip = set.allObjects; // expected-warning{{from 'NSArray<id<NSCopying>> *'}}
  ip = mutSet.allObjects; // expected-warning{{from 'NSArray<id<NSCopying>> *'}}
  ip = mutArraySet.allObjects; // expected-warning{{from 'NSArray<NSArray<id> *> *'}}
}

void test_property_write(
       NSMutableSet<NSString *> *mutStringSet,
       WidgetSet *widgetSet,
       UntypedMutableSet *untypedMutSet,
       MutableSetOfArrays<NSString *> *mutStringArraySet,
       NSMutableSet *mutSet,
       MutableSetOfArrays *mutArraySet) {
  int *ip;

  mutStringSet.allObjects = ip; // expected-warning{{to 'NSArray<NSString *> *'}}
  widgetSet.allObjects = ip; // expected-warning{{to 'NSArray<Widget *> *'}}
  untypedMutSet.allObjects = ip; // expected-warning{{to 'NSArray<id<NSCopying>> *'}}
  mutStringArraySet.allObjects = ip; // expected-warning{{to 'NSArray<NSArray<NSString *> *> *'}}
  mutSet.allObjects = ip; // expected-warning{{to 'NSArray<id<NSCopying>> *'}}
  mutArraySet.allObjects = ip; // expected-warning{{to 'NSArray<NSArray<id> *> *'}}
}
