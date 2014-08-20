#define BOOL char
#define YES __objc_yes
#define NO __objc_no
#define nil ((void*) 0)
#define NSUInteger unsigned

@class NSString, NSException, NSValue;

#import <SenTestingKit/SenTestCase.h>

extern NSString * const SenTestCaseDidStartNotification;
extern int SenSelfTestMain(void);
extern NSString *STComposeString(NSString *, ...);
