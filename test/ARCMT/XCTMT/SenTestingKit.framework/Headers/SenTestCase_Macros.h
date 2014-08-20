
#define STAssertEquals(a1, a2, description, ...) \
do { \
    @try {\
        if (strcmp(@encode(__typeof__(a1)), @encode(__typeof__(a2))) != 0) { \
            [self failWithException:([NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                         atLine:__LINE__ \
                                                withDescription:@"%@", [@"Type mismatch -- " stringByAppendingString:STComposeString(description, ##__VA_ARGS__)]])]; \
        } \
        else { \
            __typeof__(a1) a1value = (a1); \
            __typeof__(a2) a2value = (a2); \
            NSValue *a1encoded = [NSValue value:&a1value withObjCType:@encode(__typeof__(a1))]; \
            NSValue *a2encoded = [NSValue value:&a2value withObjCType:@encode(__typeof__(a2))]; \
            if (![a1encoded isEqualToValue:a2encoded]) { \
                [self failWithException:([NSException failureInEqualityBetweenValue:a1encoded \
                                                                           andValue:a2encoded \
                                                                       withAccuracy:nil \
                                                                             inFile:[NSString stringWithUTF8String:__FILE__] \
                                                                             atLine:__LINE__ \
                                                                    withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
            } \
        } \
    } \
    @catch (id anException) {\
        [self failWithException:([NSException \
                 failureInRaise:[NSString stringWithFormat:@"(%s) == (%s)", #a1, #a2] \
                      exception:anException \
                         inFile:[NSString stringWithUTF8String:__FILE__] \
                         atLine:__LINE__ \
                withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
    }\
} while(0)

#define STAssertEqualsWithAccuracy(a1, a2, accuracy, description, ...) \
do { \
    @try {\
        if (strcmp(@encode(__typeof__(a1)), @encode(__typeof__(a2))) != 0) { \
            [self failWithException:([NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                         atLine:__LINE__ \
                                                withDescription:@"%@", [@"Type mismatch -- " stringByAppendingString:STComposeString(description, ##__VA_ARGS__)]])]; \
        } \
        else { \
            __typeof__(a1) a1value = (a1); \
            __typeof__(a2) a2value = (a2); \
            __typeof__(accuracy) accuracyvalue = (accuracy); \
            if (STAbsoluteDifference(a1value, a2value) > accuracyvalue) { \
                NSValue *a1encoded = [NSValue value:&a1value withObjCType:@encode(__typeof__(a1))]; \
                NSValue *a2encoded = [NSValue value:&a2value withObjCType:@encode(__typeof__(a2))]; \
                NSValue *accuracyencoded = [NSValue value:&accuracyvalue withObjCType:@encode(__typeof__(accuracy))]; \
                [self failWithException:([NSException failureInEqualityBetweenValue:a1encoded \
                                                                           andValue:a2encoded \
                                                                       withAccuracy:accuracyencoded \
                                                                             inFile:[NSString stringWithUTF8String:__FILE__] \
                                                                             atLine:__LINE__ \
                                                                    withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
            } \
        } \
    } \
    @catch (id anException) {\
        [self failWithException:([NSException failureInRaise:[NSString stringWithFormat:@"(%s) == (%s)", #a1, #a2] \
                                                   exception:anException \
                                                      inFile:[NSString stringWithUTF8String:__FILE__] \
                                                      atLine:__LINE__ \
                                             withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
    }\
} while(0)

#define STFail(description, ...) \
[self failWithException:([NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                             atLine:__LINE__ \
                                    withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]

#define STAssertTrue(expr, description, ...) \
do { \
        BOOL _evaluatedExpression = !!(expr);\
            if (!_evaluatedExpression) {\
                NSString *_expression = [NSString stringWithUTF8String:#expr];\
                    [self failWithException:([NSException failureInCondition:_expression \
                                                                      isTrue:NO \
                                                                      inFile:[NSString stringWithUTF8String:__FILE__] \
                                                                      atLine:__LINE__ \
                                                             withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
            } \
} while (0)

#define STAssertTrueNoThrow(expr, description, ...) \
do { \
    @try {\
        BOOL _evaluatedExpression = !!(expr);\
            if (!_evaluatedExpression) {\
                NSString *_expression = [NSString stringWithUTF8String:#expr];\
                    [self failWithException:([NSException failureInCondition:_expression \
                                                                      isTrue:NO \
                                                                      inFile:[NSString stringWithUTF8String:__FILE__] \
                                                                      atLine:__LINE__ \
                                                             withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
            } \
    } \
    @catch (id anException) {\
        [self failWithException:([NSException failureInRaise:[NSString stringWithFormat:@"(%s) ", #expr] \
                                                   exception:anException \
                                                      inFile:[NSString stringWithUTF8String:__FILE__] \
                                                      atLine:__LINE__ \
                                             withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
    }\
} while (0)

#define STAssertFalse(expr, description, ...) \
do { \
        BOOL _evaluatedExpression = !!(expr);\
            if (_evaluatedExpression) {\
                NSString *_expression = [NSString stringWithUTF8String:#expr];\
                    [self failWithException:([NSException failureInCondition:_expression \
                                                                      isTrue:YES \
                                                                      inFile:[NSString stringWithUTF8String:__FILE__] \
                                                                      atLine:__LINE__ \
                                                             withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
            } \
} while (0)

#define STAssertFalseNoThrow(expr, description, ...) \
do { \
    @try {\
        BOOL _evaluatedExpression = !!(expr);\
            if (_evaluatedExpression) {\
                NSString *_expression = [NSString stringWithUTF8String:#expr];\
                    [self failWithException:([NSException failureInCondition:_expression \
                                                                      isTrue:YES \
                                                                      inFile:[NSString stringWithUTF8String:__FILE__] \
                                                                      atLine:__LINE__ \
                                                             withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
            } \
    } \
    @catch (id anException) {\
        [self failWithException:([NSException failureInRaise:[NSString stringWithFormat:@"!(%s) ", #expr] \
                                                   exception:anException \
                                                      inFile:[NSString stringWithUTF8String:__FILE__] \
                                                      atLine:__LINE__ \
                                             withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
    }\
} while (0)

#define STAssertThrows(expr, description, ...) \
do { \
    BOOL __caughtException = NO; \
    @try { \
        (expr);\
    } \
    @catch (id anException) { \
        __caughtException = YES; \
    }\
    if (!__caughtException) { \
        [self failWithException:([NSException failureInRaise:[NSString stringWithUTF8String:#expr] \
                                                   exception:nil \
                                                      inFile:[NSString stringWithUTF8String:__FILE__] \
                                                      atLine:__LINE__ \
                                             withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
    } \
} while (0)
