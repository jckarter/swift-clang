// RUN: %clang_cc1 -std=c++11 -fsyntax-only -Wdocumentation -Wdocumentation-pedantic -verify %s

// This file contains lots of corner cases, so ensure that XML we generate is not invalid.
// RUN: c-index-test -test-load-source all -comments-xml-schema=%S/../../bindings/xml/comment-xml-schema.rng %s | FileCheck %s -check-prefix=WRONG
// WRONG-NOT: CommentXMLInvalid

// expected-warning@+1 {{expected quoted string after equals sign}}
/// <a href=>
int test_html1(int);

// expected-warning@+1 {{expected quoted string after equals sign}}
/// <a href==>
int test_html2(int);

// expected-warning@+2 {{expected quoted string after equals sign}}
// expected-warning@+1 {{HTML start tag prematurely ended, expected attribute name or '>'}}
/// <a href= blah
int test_html3(int);

// expected-warning@+1 {{HTML start tag prematurely ended, expected attribute name or '>'}}
/// <a =>
int test_html4(int);

// expected-warning@+1 {{HTML start tag prematurely ended, expected attribute name or '>'}}
/// <a "aaa">
int test_html5(int);

// expected-warning@+1 {{HTML start tag prematurely ended, expected attribute name or '>'}}
/// <a a="b" =>
int test_html6(int);

// expected-warning@+1 {{HTML start tag prematurely ended, expected attribute name or '>'}}
/// <a a="b" "aaa">
int test_html7(int);

// expected-warning@+1 {{HTML start tag prematurely ended, expected attribute name or '>'}}
/// <a a="b" =
int test_html8(int);

// expected-warning@+2 {{HTML start tag prematurely ended, expected attribute name or '>'}} expected-note@+1 {{HTML tag started here}}
/** Aaa bbb<img ddd eee
 * fff ggg.
 */
int test_html9(int);

// expected-warning@+1 {{HTML start tag prematurely ended, expected attribute name or '>'}}
/** Aaa bbb<img ddd eee 42%
 * fff ggg.
 */
int test_html10(int);

// expected-warning@+1 {{HTML end tag 'br' is forbidden}}
/// <br></br>
int test_html11(int);

/// <blockquote>Meow</blockquote>
int test_html_nesting1(int);

/// <b><i>Meow</i></b>
int test_html_nesting2(int);

/// <p>Aaa<br>
/// Bbb</p>
int test_html_nesting3(int);

/// <p>Aaa<br />
/// Bbb</p>
int test_html_nesting4(int);

// expected-warning@+1 {{HTML end tag does not match any start tag}}
/// <b><i>Meow</a>
int test_html_nesting5(int);

// expected-warning@+2 {{HTML start tag 'i' closed by 'b'}}
// expected-warning@+1 {{HTML end tag does not match any start tag}}
/// <b><i>Meow</b></b>
int test_html_nesting6(int);

// expected-warning@+2 {{HTML start tag 'i' closed by 'b'}}
// expected-warning@+1 {{HTML end tag does not match any start tag}}
/// <b><i>Meow</b></i>
int test_html_nesting7(int);


// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\returns Aaa
int test_block_command1(int);

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief \returns Aaa
int test_block_command2(int);

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief
/// \returns Aaa
int test_block_command3(int);

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief
///
/// \returns Aaa
int test_block_command4(int);

// There is trailing whitespace on one of the following lines, don't remove it!
// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief
/// 
/// \returns Aaa
int test_block_command5(int);

/// \brief \c Aaa
int test_block_command6(int);

// expected-warning@+5 {{duplicated command '\brief'}} expected-note@+1 {{previous command '\brief' here}}
/// \brief Aaa
///
/// Bbb
///
/// \brief Ccc
int test_duplicate_brief1(int);

// expected-warning@+5 {{duplicated command '\short'}} expected-note@+1 {{previous command '\short' here}}
/// \short Aaa
///
/// Bbb
///
/// \short Ccc
int test_duplicate_brief2(int);

// expected-warning@+5 {{duplicated command '\brief'}} expected-note@+1 {{previous command '\short' (an alias of '\brief') here}}
/// \short Aaa
///
/// Bbb
///
/// \brief Ccc
int test_duplicate_brief3(int);


// expected-warning@+5 {{duplicated command '\return'}} expected-note@+1 {{previous command '\return' here}}
/// \return Aaa
///
/// Bbb
///
/// \return Ccc
int test_duplicate_returns1(int);

// expected-warning@+5 {{duplicated command '\returns'}} expected-note@+1 {{previous command '\returns' here}}
/// \returns Aaa
///
/// Bbb
///
/// \returns Ccc
int test_duplicate_returns2(int);

// expected-warning@+5 {{duplicated command '\result'}} expected-note@+1 {{previous command '\result' here}}
/// \result Aaa
///
/// Bbb
///
/// \result Ccc
int test_duplicate_returns3(int);

// expected-warning@+5 {{duplicated command '\return'}} expected-note@+1 {{previous command '\returns' (an alias of '\return') here}}
/// \returns Aaa
///
/// Bbb
///
/// \return Ccc
int test_duplicate_returns4(int);


// expected-warning@+1 {{'\param' command used in a comment that is not attached to a function declaration}}
/// \param a Blah blah.
int test_param1;

// expected-warning@+1 {{empty paragraph passed to '\param' command}}
/// \param
/// \param a Blah blah.
int test_param2(int a);

// expected-warning@+1 {{empty paragraph passed to '\param' command}}
/// \param a
int test_param3(int a);

/// \param a Blah blah.
int test_param4(int a);

/// \param [in] a Blah blah.
int test_param5(int a);

/// \param [out] a Blah blah.
int test_param6(int a);

/// \param [in,out] a Blah blah.
int test_param7(int a);

// expected-warning@+1 {{whitespace is not allowed in parameter passing direction}}
/// \param [ in ] a Blah blah.
int test_param8(int a);

// expected-warning@+1 {{whitespace is not allowed in parameter passing direction}}
/// \param [in, out] a Blah blah.
int test_param9(int a);

// expected-warning@+1 {{unrecognized parameter passing direction, valid directions are '[in]', '[out]' and '[in,out]'}}
/// \param [ junk] a Blah blah.
int test_param10(int a);

// expected-warning@+1 {{parameter 'a' not found in the function declaration}}
/// \param a Blah blah.
int test_param11();

// expected-warning@+1 {{parameter 'A' not found in the function declaration}} expected-note@+1 {{did you mean 'a'?}}
/// \param A Blah blah.
int test_param12(int a);

// expected-warning@+1 {{parameter 'aab' not found in the function declaration}} expected-note@+1 {{did you mean 'aaa'?}}
/// \param aab Blah blah.
int test_param13(int aaa, int bbb);

// expected-warning@+1 {{parameter 'aab' not found in the function declaration}}
/// \param aab Blah blah.
int test_param14(int bbb, int ccc);

class C {
  // expected-warning@+1 {{parameter 'aaa' not found in the function declaration}}
  /// \param aaa Blah blah.
  C(int bbb, int ccc);

  // expected-warning@+1 {{parameter 'aaa' not found in the function declaration}}
  /// \param aaa Blah blah.
 int test_param15(int bbb, int ccc);
};

// expected-warning@+1 {{parameter 'aab' not found in the function declaration}}
/// \param aab Blah blah.
template<typename T>
void test_param16(int bbb, int ccc);

// expected-warning@+3 {{parameter 'a' is already documented}}
// expected-note@+1 {{previous documentation}}
/// \param a Aaa.
/// \param a Aaa.
int test_param17(int a);

// expected-warning@+4 {{parameter 'x2' is already documented}}
// expected-note@+2 {{previous documentation}}
/// \param x1 Aaa.
/// \param x2 Bbb.
/// \param x2 Ccc.
int test_param18(int x1, int x2, int x3);


// expected-warning@+1 {{'\tparam' command used in a comment that is not attached to a template declaration}}
/// \tparam T Aaa
int test_tparam1;

// expected-warning@+1 {{'\tparam' command used in a comment that is not attached to a template declaration}}
/// \tparam T Aaa
void test_tparam2(int aaa);

// expected-warning@+1 {{empty paragraph passed to '\tparam' command}}
/// \tparam
/// \param aaa Blah blah
template<typename T>
void test_tparam3(T aaa);

// expected-warning@+1 {{template parameter 'T' not found in the template declaration}} expected-note@+1 {{did you mean 'TT'?}}
/// \tparam T Aaa
template<typename TT>
void test_tparam4(TT aaa);

// expected-warning@+1 {{template parameter 'T' not found in the template declaration}} expected-note@+1 {{did you mean 'TT'?}}
/// \tparam T Aaa
template<typename TT>
class test_tparam5 {
  // expected-warning@+1 {{template parameter 'T' not found in the template declaration}} expected-note@+1 {{did you mean 'TTT'?}}
  /// \tparam T Aaa
  template<typename TTT>
  void test_tparam6(TTT aaa);
};

/// \tparam T1 Aaa
/// \tparam T2 Bbb
template<typename T1, typename T2>
void test_tparam7(T1 aaa, T2 bbb);

// expected-warning@+1 {{template parameter 'SomTy' not found in the template declaration}} expected-note@+1 {{did you mean 'SomeTy'?}}
/// \tparam SomTy Aaa
/// \tparam OtherTy Bbb
template<typename SomeTy, typename OtherTy>
void test_tparam8(SomeTy aaa, OtherTy bbb);

// expected-warning@+2 {{template parameter 'T1' is already documented}} expected-note@+1 {{previous documentation}}
/// \tparam T1 Aaa
/// \tparam T1 Bbb
template<typename T1, typename T2>
void test_tparam9(T1 aaa, T2 bbb);

/// \tparam T Aaa
/// \tparam TT Bbb
template<template<typename T> class TT>
void test_tparam10(TT<int> aaa);

/// \tparam T Aaa
/// \tparam TT Bbb
/// \tparam TTT Ccc
template<template<template<typename T> class TT, class C> class TTT>
void test_tparam11();

/// \tparam I Aaa
template<int I>
void test_tparam12();

template<typename T, typename U>
class test_tparam13 { };

/// \tparam T Aaa
template<typename T>
using test_tparam14 = test_tparam13<T, int>;

// expected-warning@+1 {{template parameter 'U' not found in the template declaration}} expected-note@+1 {{did you mean 'T'?}}
/// \tparam U Aaa
template<typename T>
using test_tparam15 = test_tparam13<T, int>;

// no-warning
/// \returns Aaa
int test_returns_right_decl_1(int);

class test_returns_right_decl_2 {
  // no-warning
  /// \returns Aaa
  int test_returns_right_decl_3(int);
};

// no-warning
/// \returns Aaa
template<typename T>
int test_returns_right_decl_4(T aaa);

// no-warning
/// \returns Aaa
template<>
int test_returns_right_decl_4(int aaa);

/// \returns Aaa
template<typename T>
T test_returns_right_decl_5(T aaa);

// expected-warning@+1 {{'\returns' command used in a comment that is not attached to a function or method declaration}}
/// \returns Aaa
int test_returns_wrong_decl_1;

// expected-warning@+1 {{'\return' command used in a comment that is not attached to a function or method declaration}}
/// \return Aaa
int test_returns_wrong_decl_2;

// expected-warning@+1 {{'\result' command used in a comment that is not attached to a function or method declaration}}
/// \result Aaa
int test_returns_wrong_decl_3;

// expected-warning@+1 {{'\returns' command used in a comment that is attached to a function returning void}}
/// \returns Aaa
void test_returns_wrong_decl_4(int);

// expected-warning@+1 {{'\returns' command used in a comment that is attached to a function returning void}}
/// \returns Aaa
template<typename T>
void test_returns_wrong_decl_5(T aaa);

// expected-warning@+1 {{'\returns' command used in a comment that is attached to a function returning void}}
/// \returns Aaa
template<>
void test_returns_wrong_decl_5(int aaa);

// expected-warning@+1 {{'\returns' command used in a comment that is not attached to a function or method declaration}}
/// \returns Aaa
struct test_returns_wrong_decl_6 { };

// expected-warning@+1 {{'\returns' command used in a comment that is not attached to a function or method declaration}}
/// \returns Aaa
class test_returns_wrong_decl_7 {
  // expected-warning@+1 {{'\returns' command used in a comment that is attached to a constructor}}
  /// \returns Aaa
  test_returns_wrong_decl_7();

  // expected-warning@+1 {{'\returns' command used in a comment that is attached to a destructor}}
  /// \returns Aaa
  ~test_returns_wrong_decl_7();
};

// expected-warning@+1 {{'\returns' command used in a comment that is not attached to a function or method declaration}}
/// \returns Aaa
enum test_returns_wrong_decl_8 {
  // expected-warning@+1 {{'\returns' command used in a comment that is not attached to a function or method declaration}}
  /// \returns Aaa
  test_returns_wrong_decl_9
};

// expected-warning@+1 {{'\returns' command used in a comment that is not attached to a function or method declaration}}
/// \returns Aaa
namespace test_returns_wrong_decl_10 { };


// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
int test1; ///< \brief\author Aaa

// expected-warning@+2 {{empty paragraph passed to '\brief' command}}
// expected-warning@+2 {{empty paragraph passed to '\brief' command}}
int test2, ///< \brief\author Aaa
    test3; ///< \brief\author Aaa

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
int test4; ///< \brief
           ///< \author Aaa


// Check that we attach the comment to the declaration during parsing in the
// following cases.  The test is based on the fact that we don't parse
// documentation comments that are not attached to anything.

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
int test_attach1;

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
int test_attach2(int);

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
struct test_attach3 {
  // expected-warning@+1 {{empty paragraph passed to '\brief' command}}
  /// \brief\author Aaa
  int test_attach4;

  // expected-warning@+1 {{empty paragraph passed to '\brief' command}}
  int test_attach5; ///< \brief\author Aaa

  // expected-warning@+1 {{empty paragraph passed to '\brief' command}}
  /// \brief\author Aaa
  int test_attach6(int);
};

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
class test_attach7 {
  // expected-warning@+1 {{empty paragraph passed to '\brief' command}}
  /// \brief\author Aaa
  int test_attach8;

  // expected-warning@+1 {{empty paragraph passed to '\brief' command}}
  int test_attach9; ///< \brief\author Aaa

  // expected-warning@+1 {{empty paragraph passed to '\brief' command}}
  /// \brief\author Aaa
  int test_attach10(int);
};

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
enum test_attach9 {
  // expected-warning@+1 {{empty paragraph passed to '\brief' command}}
  /// \brief\author Aaa
  test_attach10,

  // expected-warning@+1 {{empty paragraph passed to '\brief' command}}
  test_attach11 ///< \brief\author Aaa
};

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
struct test_noattach12 *test_attach13;

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
typedef struct test_noattach14 *test_attach15;

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
typedef struct test_attach16 { int a; } test_attach17;

struct S { int a; };

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
struct S *test_attach18;

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
typedef struct S *test_attach19;

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
struct test_attach20;

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
typedef struct test_attach21 {
  // expected-warning@+1 {{empty paragraph passed to '\brief' command}}
  /// \brief\author Aaa
  int test_attach22;
} test_attach23;

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
namespace test_attach24 {
  // expected-warning@+1 {{empty paragraph passed to '\brief' command}}
  /// \brief\author Aaa
  namespace test_attach25 {
  }
}

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
/// \tparam T Aaa
template<typename T>
void test_attach26(T aaa);

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
/// \tparam T Aaa
template<typename T, typename U>
void test_attach27(T aaa, U bbb);

// expected-warning@+2 {{empty paragraph passed to '\brief' command}}
// expected-warning@+2 {{template parameter 'T' not found in the template declaration}}
/// \brief\author Aaa
/// \tparam T Aaa
template<>
void test_attach27(int aaa, int bbb);

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
/// \tparam T Aaa
template<typename T>
class test_attach28 {
  T aaa;
};

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
using test_attach29 = test_attach28<int>;

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
/// \tparam T Aaa
template<typename T, typename U>
class test_attach30 { };

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
/// \tparam T Aaa
template<typename T>
class test_attach30<T, int> { };

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
template<>
class test_attach30<int, int> { };

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
template<typename T>
using test_attach31 = test_attach30<T, int>;

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
/// \tparam T Aaa
template<typename T, typename U, typename V>
class test_attach32 { };

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
/// \tparam T Aaa
template<typename T, typename U>
class test_attach32<T, U, int> { };

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
/// \tparam T Aaa
template<typename T>
class test_attach32<T, int, int> { };

// expected-warning@+2 {{empty paragraph passed to '\brief' command}}
// expected-warning@+2 {{template parameter 'T' not found in the template declaration}}
/// \brief\author Aaa
/// \tparam T Aaa
template<>
class test_attach32<int, int, int> { };

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
class test_attach33 {
  // expected-warning@+1 {{empty paragraph passed to '\brief' command}}
  /// \brief\author Aaa
  /// \tparam T Aaa
  template<typename T, typename U>
  void test_attach34(T aaa, U bbb);
};

template<typename T>
class test_attach35 {
  // expected-warning@+2 {{empty paragraph passed to '\brief' command}}
  // expected-warning@+2 {{template parameter 'T' not found in the template declaration}}
  /// \brief\author Aaa
  /// \tparam T Aaa
  template<typename TT, typename UU>
  void test_attach36(TT aaa, UU bbb);
};

// expected-warning@+2 {{empty paragraph passed to '\brief' command}}
// expected-warning@+2 {{template parameter 'T' not found in the template declaration}}
/// \brief\author Aaa
/// \tparam T Aaa
template<> template<>
void test_attach35<int>::test_attach36(int aaa, int bbb) {}

template<typename T>
class test_attach37 {
  // expected-warning@+2 {{empty paragraph passed to '\brief' command}}
  // expected-warning@+2 {{'\tparam' command used in a comment that is not attached to a template declaration}}
  /// \brief\author Aaa
  /// \tparam T Aaa
  void test_attach38(int aaa, int bbb);

  void test_attach39(int aaa, int bbb);
};

// expected-warning@+2 {{empty paragraph passed to '\brief' command}}
// expected-warning@+2 {{template parameter 'T' not found in the template declaration}}
/// \brief\author Aaa
/// \tparam T Aaa
template<>
void test_attach37<int>::test_attach38(int aaa, int bbb) {}

// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \brief\author Aaa
/// \tparam T Aaa
template<typename T>
void test_attach37<T>::test_attach39(int aaa, int bbb) {}

// We used to emit warning that parameter 'a' is not found because we parsed
// the comment in context of the redeclaration which does not have parameter
// names.
template <typename T>
struct test_attach38 {
  /*!
    \param a  First param
    \param b  Second param
  */
  template <typename B>
  void test_attach39(T a, B b);
};

template <>
template <typename B>
void test_attach38<int>::test_attach39(int, B);


// PR13411, reduced.  We used to crash on this.
/**
 * @code Aaa.
 */
void test_nocrash1(int);

// We used to crash on this.
// expected-warning@+2 {{empty paragraph passed to '\param' command}}
// expected-warning@+1 {{empty paragraph passed to '\brief' command}}
/// \param\brief
void test_nocrash2(int);

// PR13593, example 1 and 2

/**
* Bla.
*/
template <typename>
void test_nocrash3();

/// Foo
template <typename, typename>
void test_nocrash4() { }

template <typename>
void test_nocrash3()
{
}

// PR13593, example 3

/**
 * aaa
 */
template <typename T>
inline T test_nocrash5(T a1)
{
    return a1;
}

///
//,

inline void test_nocrash6()
{
    test_nocrash5(1);
}

// We used to crash on this.

/*!
  Blah.
*/
typedef const struct test_nocrash7 * test_nocrash8;

