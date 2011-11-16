@protocol P @end

@interface NSMutableArray
- (id)objectAtIndexedSubscript:(unsigned int)index;
- (void)setObject:(id)object atIndexedSubscript:(unsigned int)index;
@end

@interface NSMutableDictionary
- (id)objectForKeyedSubscript:(id)key;
- (void)setObject:(id)object forKeyedSubscript:(id)key;
@end

