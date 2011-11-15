@protocol P @end

@interface NSMutableArray
- (id)objectAtIndexedSubscript:(unsigned int)index;
- (void)objectAtIndexedSubscript:(unsigned int)index put:(id)object;
@end

@interface NSMutableDictionary
- (id)objectForKeyedSubscript:(id)key;
- (void)setObject:(id)object forKeyedSubscript:(id)key;
@end

