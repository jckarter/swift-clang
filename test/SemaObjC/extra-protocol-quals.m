// RUN: %clang_cc1 -fsyntax-only %s -verify

@protocol NSObject
@end

@protocol NSCopying
@end

__attribute__((objc_root_class))
@interface NSObject <NSObject>
@end

@interface NSString : NSObject <NSCopying>
@end

typedef NSObject<NSObject> extraneous1; // expected-warning{{class 'NSObject' always conforms to the protocol 'NSObject'}}
typedef NSString<NSCopying, // expected-warning{{class 'NSString' always conforms to the protocol 'NSCopying'}}
                 NSObject> extraneous2; // expected-warning{{class 'NSString' always conforms to the protocol 'NSObject'}}
