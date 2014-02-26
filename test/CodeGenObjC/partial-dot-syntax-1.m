// RUN: %clang_cc1 -emit-llvm -o %t %s
// rdar://16076729

int printf(const char *, ...);

@partial_interface Root;
@partial_interface Top0 : Root;
@partial_interface Bot0 : Top0;
@partial_interface Top1 : Root;
@partial_interface Bot1 : Top1;

__attribute((objc_complete_definition))
@interface Root
-(id) alloc;
-(id) init;
@end

// Property above methods...

__attribute((objc_complete_definition))
@interface Top0 : Root
@property(getter=_getX,setter=_setX:) int x;
@end

__attribute((objc_complete_definition))
@interface Bot0 : Top0
-(int) x;
-(void) setX: (int) arg;
@end

@implementation Top0
-(int) _getX {
  printf("-[ Top0 _getX ]\n");
  return 0;
}
-(void) _setX: (int) arg {
  printf("-[ Top0 _setX: %d ]\n", arg);
}
@end

@implementation Bot0
-(int) x {
  printf("-[ Bot0 _getX ]\n");
  return 0;
}
-(void) setX: (int) arg {
  printf("-[ Bot0 _setX: %d ]\n", arg);
}
@end

// Methods above property...

__attribute((objc_complete_definition))
@interface Top1 : Root
-(int) x;
-(void) setX: (int) arg;
@end

__attribute((objc_complete_definition))
@interface Bot1 : Top1
@property(getter=_getX,setter=_setX:) int x;
@end

@implementation Top1
-(int) x {
  printf("-[ Top1 x ]\n");
  return 0;
}
-(void) setX: (int) arg {
  printf("-[ Top1 setX: %d ]\n", arg);
}
@end

@implementation Bot1
-(int) _getX {
  printf("-[ Bot1 _getX ]\n");
  return 0;
}
-(void) _setX: (int) arg {
  printf("-[ Bot1 _setX: %d ]\n", arg);
}
@end

// Mixed setter & getter (variant 1)

@partial_interface Top2 : Root;
@partial_interface Bot2 : Top2;
@partial_interface Top3 : Root;
@partial_interface Bot3 : Top3;

__attribute((objc_complete_definition))
@interface Top2 : Root
-(int) x;
-(void) _setX: (int) arg;
@end

__attribute((objc_complete_definition))
@interface Bot2 : Top2
@property(getter=_getX,setter=_setX:) int x;
@end

@implementation Top2
-(int) x {
  printf("-[ Top2 x ]\n");
  return 0;
}
-(void) _setX: (int) arg {
  printf("-[ Top2 _setX: %d ]\n", arg);
}
@end

@implementation Bot2
-(int) _getX {
  printf("-[ Bot2 _getX ]\n");
  return 0;
}
-(void) setX: (int) arg {
  printf("-[ Bot2 setX: %d ]\n", arg);
}
@end

// Mixed setter & getter (variant 2)

__attribute((objc_complete_definition))
@interface Top3 : Root
-(int) _getX;
-(void) setX: (int) arg;
@end

__attribute((objc_complete_definition))
@interface Bot3 : Top3
@property(getter=_getX,setter=_setX:) int x;
@end

@implementation Top3
-(int) _getX {
  printf("-[ Top3 _getX ]\n");
  return 0;
}
-(void) setX: (int) arg {
  printf("-[ Top3 setX: %d ]\n", arg);
}
@end

@implementation Bot3
-(int) x {
  printf("-[ Bot3 x ]\n");
  return 0;
}
-(void) _setX: (int) arg {
  printf("-[ Bot3 _setX: %d ]\n", arg);
}
@end

// Mixed setter & getter (variant 3)

@partial_interface Top4 : Root;
@partial_interface Bot4 : Top4;
@partial_interface Top5 : Root;
@partial_interface Bot5 : Top5;

__attribute((objc_complete_definition))
@interface Top4 : Root
@property(getter=_getX,setter=_setX:) int x;
@end

__attribute((objc_complete_definition))
@interface Bot4 : Top4
-(int) _getX;
-(void) setX: (int) arg;
@end

@implementation Top4
-(int) x {
  printf("-[ Top4 x ]\n");
  return 0;
}
-(void) _setX: (int) arg {
  printf("-[ Top4 _setX: %d ]\n", arg);
}
@end

@implementation Bot4
-(int) _getX {
  printf("-[ Bot4 _getX ]\n");
  return 0;
}
-(void) setX: (int) arg {
  printf("-[ Bot4 setX: %d ]\n", arg);
}
@end

// Mixed setter & getter (variant 4)

__attribute((objc_complete_definition))
@interface Top5 : Root
@property(getter=_getX,setter=_setX:) int x;
@end

__attribute((objc_complete_definition))
@interface Bot5 : Top5
-(int) x;
-(void) _setX: (int) arg;
@end

@implementation Top5
-(int) _getX {
  printf("-[ Top5 _getX ]\n");
  return 0;
}
-(void) setX: (int) arg {
  printf("-[ Top5 setX: %d ]\n", arg);
}
@end

@implementation Bot5
-(int) x {
  printf("-[ Bot5 x ]\n");
  return 0;
}
-(void) _setX: (int) arg {
  printf("-[ Bot5 _setX: %d ]\n", arg);
}
@end

// Mixed level calls (variant 1)
@partial_interface Top6 : Root;
@partial_interface Bot6 : Top6;

__attribute((objc_complete_definition))
@interface Top6 : Root
-(int) x;
@end

__attribute((objc_complete_definition))
@interface Bot6 : Top6
-(void) setX: (int) arg;
@end

@implementation Top6
-(int) x {
  printf("-[ Top6 x ]\n");
  return 0;
}
@end

@implementation Bot6
-(void) setX: (int) arg {
  printf("-[ Bot5 setX: %d ]\n", arg);
}
@end

// Mixed level calls (variant 1)

@partial_interface Top7 : Root;
@partial_interface Bot7 : Top7;

__attribute((objc_complete_definition))
@interface Top7 : Root
-(void) setX: (int) arg;
@end

__attribute((objc_complete_definition))
@interface Bot7 : Top7
-(int) x;
@end

@implementation Top7
-(void) setX: (int) arg {
  printf("-[ Top7 setX: %d ]\n", arg);
}
@end

@implementation Bot7
-(int) x {
  printf("-[ Bot7 x ]\n");
  return 0;
}
@end

//

// FIXME: Two more (thats it?) interesting cases. Method access on
// getter w/o setter and method access on setter w/o getter.

int main() {
#define test(N) { \
  Bot##N *ob = [[Bot##N alloc] init]; \
  int x = ob.x; \
  ob.x = 10; }

  test(0);
  test(1);
  test(2);
  test(3);
  test(4);
  test(5);
  //  test(6);
  //  test(7);

  return 0;
}

