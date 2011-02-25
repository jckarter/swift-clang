// RUN: %clang_cc1 -analyze -analyzer-checker=core.experimental.UnreachableCode,core.experimental.CastSize -analyzer-check-objc-mem -analyzer-experimental-checks -analyzer-store=region -verify %s
typedef __typeof(sizeof(int)) size_t;
void *malloc(size_t);
void free(void *);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nmemb, size_t size);
void __attribute((ownership_returns(malloc))) *my_malloc(size_t);
void __attribute((ownership_takes(malloc, 1))) my_free(void *);
void __attribute((ownership_returns(malloc, 1))) *my_malloc2(size_t);
void __attribute((ownership_holds(malloc, 1))) my_hold(void *);

// Duplicate attributes are silly, but not an error.
// Duplicate attribute has no extra effect.
// If two are of different kinds, that is an error and reported as such. 
void __attribute((ownership_holds(malloc, 1)))
__attribute((ownership_holds(malloc, 1)))
__attribute((ownership_holds(malloc, 3))) my_hold2(void *, void *, void *);
void *my_malloc3(size_t);
void *myglobalpointer;
struct stuff {
  void *somefield;
};
struct stuff myglobalstuff;

void f1() {
  int *p = malloc(12);
  return; // expected-warning{{Allocated memory never released. Potential memory leak.}}
}

void f2() {
  int *p = malloc(12);
  free(p);
  free(p); // expected-warning{{Try to free a memory block that has been released}}
}

// ownership attributes tests
void naf1() {
  int *p = my_malloc3(12);
  return; // no-warning
}

void n2af1() {
  int *p = my_malloc2(12);
  return; // expected-warning{{Allocated memory never released. Potential memory leak.}}
}

void af1() {
  int *p = my_malloc(12);
  return; // expected-warning{{Allocated memory never released. Potential memory leak.}}
}

void af1_b() {
  int *p = my_malloc(12); // expected-warning{{Allocated memory never released. Potential memory leak.}}
}

void af1_c() {
  myglobalpointer = my_malloc(12); // no-warning
}

void af1_d() {
  struct stuff mystuff;
  mystuff.somefield = my_malloc(12); // expected-warning{{Allocated memory never released. Potential memory leak.}}
}

// Test that we can pass out allocated memory via pointer-to-pointer.
void af1_e(void **pp) {
  *pp = my_malloc(42); // no-warning
}

void af1_f(struct stuff *somestuff) {
  somestuff->somefield = my_malloc(12); // no-warning
}

// Allocating memory for a field via multiple indirections to our arguments is OK.
void af1_g(struct stuff **pps) {
  *pps = my_malloc(sizeof(struct stuff)); // no-warning
  (*pps)->somefield = my_malloc(42); // no-warning
}

void af2() {
  int *p = my_malloc(12);
  my_free(p);
  free(p); // expected-warning{{Try to free a memory block that has been released}}
}

void af2b() {
  int *p = my_malloc(12);
  free(p);
  my_free(p); // expected-warning{{Try to free a memory block that has been released}}
}

void af2c() {
  int *p = my_malloc(12);
  free(p);
  my_hold(p); // expected-warning{{Try to free a memory block that has been released}}
}

void af2d() {
  int *p = my_malloc(12);
  free(p);
  my_hold2(0, 0, p); // expected-warning{{Try to free a memory block that has been released}}
}

// No leak if malloc returns null.
void af2e() {
  int *p = my_malloc(12);
  if (!p)
    return; // no-warning
  free(p); // no-warning
}

// This case would inflict a double-free elsewhere.
// However, this case is considered an analyzer bug since it causes false-positives.
void af3() {
  int *p = my_malloc(12);
  my_hold(p);
  free(p); // no-warning
}

// This case would inflict a double-free elsewhere.
// However, this case is considered an analyzer bug since it causes false-positives.
int * af4() {
  int *p = my_malloc(12);
  my_free(p);
  return p; // no-warning
}

// This case is (possibly) ok, be conservative
int * af5() {
  int *p = my_malloc(12);
  my_hold(p);
  return p; // no-warning
}



// This case tests that storing malloc'ed memory to a static variable which is
// then returned is not leaked.  In the absence of known contracts for functions
// or inter-procedural analysis, this is a conservative answer.
int *f3() {
  static int *p = 0;
  p = malloc(12); 
  return p; // no-warning
}

// This case tests that storing malloc'ed memory to a static global variable
// which is then returned is not leaked.  In the absence of known contracts for
// functions or inter-procedural analysis, this is a conservative answer.
static int *p_f4 = 0;
int *f4() {
  p_f4 = malloc(12); 
  return p_f4; // no-warning
}

int *f5() {
  int *q = malloc(12);
  q = realloc(q, 20);
  return q; // no-warning
}

void f6() {
  int *p = malloc(12);
  if (!p)
    return; // no-warning
  else
    free(p);
}

char *doit2();
void pr6069() {
  char *buf = doit2();
  free(buf);
}

void pr6293() {
  free(0);
}

void f7() {
  char *x = (char*) malloc(4);
  free(x);
  x[0] = 'a'; // expected-warning{{Use dynamically allocated memory after it is freed.}}
}

void PR6123() {
  int *x = malloc(11); // expected-warning{{Cast a region whose size is not a multiple of the destination type size.}}
}

void PR7217() {
  int *buf = malloc(2); // expected-warning{{Cast a region whose size is not a multiple of the destination type size.}}
  buf[1] = 'c'; // not crash
}

void mallocCastToVoid() {
  void *p = malloc(2);
  const void *cp = p; // not crash
  free(p);
}

void mallocCastToFP() {
  void *p = malloc(2);
  void (*fp)() = p; // not crash
  free(p);
}

// This tests that malloc() buffers are undefined by default
char mallocGarbage () {
	char *buf = malloc(2);
	char result = buf[1]; // expected-warning{{undefined}}
	free(buf);
	return result;
}

// This tests that calloc() buffers need to be freed
void callocNoFree () {
  char *buf = calloc(2,2);
  return; // expected-warning{{never released}}
}

// These test that calloc() buffers are zeroed by default
char callocZeroesGood () {
	char *buf = calloc(2,2);
	char result = buf[3]; // no-warning
	if (buf[1] == 0) {
	  free(buf);
	}
	return result; // no-warning
}

char callocZeroesBad () {
	char *buf = calloc(2,2);
	char result = buf[3]; // no-warning
	if (buf[1] != 0) {
	  free(buf); // expected-warning{{never executed}}
	}
	return result; // expected-warning{{never released}}
}
