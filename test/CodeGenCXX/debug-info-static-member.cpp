// RUN: %clangxx -target x86_64-unknown-unknown -g %s -emit-llvm -S -o - | FileCheck %s
// PR14471

enum X {
  Y
};
class C
{
  static int a;
  const static bool const_a = true;
protected:
  static int b;
  const static float const_b = 3.14;
public:
  static int c;
  const static int const_c = 18;
  int d;
  static X x_a;
};

int C::a = 4;
int C::b = 2;
int C::c = 1;

int main()
{
        C instance_C;
        instance_C.d = 8;
        return C::c;
}

// The definition of C::a drives the emission of class C, which is
// why the definition of "a" comes before the declarations while
// "b" and "c" come after.

// CHECK: !MDCompositeType(tag: DW_TAG_enumeration_type, name: "X"{{.*}}, identifier: "_ZTS1X")
// CHECK: !MDCompositeType(tag: DW_TAG_class_type, name: "C"{{.*}}, identifier: "_ZTS1C")
//
// CHECK: ![[DECL_A:[0-9]+]] = !MDDerivedType(tag: DW_TAG_member, name: "a"
// CHECK-NOT:                                 size:
// CHECK-NOT:                                 align:
// CHECK-NOT:                                 offset:
// CHECK-SAME:                                flags: DIFlagStaticMember)
//
// CHECK: !MDDerivedType(tag: DW_TAG_member, name: "const_a"
// CHECK-NOT:            size:
// CHECK-NOT:            align:
// CHECK-NOT:            offset:
// CHECK-SAME:           flags: DIFlagStaticMember,
// CHECK-SAME:           extraData: i1 true)
//
// CHECK: ![[DECL_B:[0-9]+]] = !MDDerivedType(tag: DW_TAG_member, name: "b"
// CHECK-NOT:                                 size:
// CHECK-NOT:                                 align:
// CHECK-NOT:                                 offset:
// CHECK-SAME:                                flags: DIFlagProtected | DIFlagStaticMember)
//
// CHECK: !MDDerivedType(tag: DW_TAG_member, name: "const_b"
// CHECK-NOT:            size:
// CHECK-NOT:            align:
// CHECK-NOT:            offset:
// CHECK-SAME:           flags: DIFlagProtected | DIFlagStaticMember,
// CHECK-SAME:           extraData: float 0x{{.*}})
//
// CHECK: ![[DECL_C:[0-9]+]] = !MDDerivedType(tag: DW_TAG_member, name: "c"
// CHECK-NOT:                                 size:
// CHECK-NOT:                                 align:
// CHECK-NOT:                                 offset:
// CHECK-SAME:                                flags: DIFlagPublic | DIFlagStaticMember)
//
// CHECK: !MDDerivedType(tag: DW_TAG_member, name: "const_c"
// CHECK-NOT:            size:
// CHECK-NOT:            align:
// CHECK-NOT:            offset:
// CHECK-SAME:           flags: DIFlagPublic | DIFlagStaticMember,
// CHECK-SAME:           extraData: i32 18)
//
// CHECK: !MDDerivedType(tag: DW_TAG_member, name: "x_a"
// CHECK-SAME:           flags: DIFlagPublic | DIFlagStaticMember)

// CHECK: !MDCompositeType(tag: DW_TAG_structure_type, name: "static_decl_templ<int>"
// CHECK-NOT:              DIFlagFwdDecl
// CHECK-SAME:             ){{$}}
// CHECK: !MDDerivedType(tag: DW_TAG_member, name: "static_decl_templ_var"

// CHECK: [[NS_X:![0-9]+]] = !MDNamespace(name: "x"

// Test this in an anonymous namespace to ensure the type is retained even when
// it doesn't get automatically retained by the string type reference machinery.
namespace {
struct anon_static_decl_struct {
  static const int anon_static_decl_var = 117;
};
}


// CHECK: !MDCompositeType(tag: DW_TAG_structure_type, name: "anon_static_decl_struct"
// CHECK: !MDDerivedType(tag: DW_TAG_member, name: "anon_static_decl_var"

int ref() {
  return anon_static_decl_struct::anon_static_decl_var;
}

template<typename T>
struct static_decl_templ {
  static const int static_decl_templ_var = 7;
};

template<typename T>
const int static_decl_templ<T>::static_decl_templ_var;

int static_decl_templ_ref() {
  return static_decl_templ<int>::static_decl_templ_var;
}

// CHECK: !MDGlobalVariable(name: "a", {{.*}}variable: i32* @_ZN1C1aE, declaration: ![[DECL_A]])
// CHECK: !MDGlobalVariable(name: "b", {{.*}}variable: i32* @_ZN1C1bE, declaration: ![[DECL_B]])
// CHECK: !MDGlobalVariable(name: "c", {{.*}}variable: i32* @_ZN1C1cE, declaration: ![[DECL_C]])

// CHECK-NOT: !MDGlobalVariable(name: "anon_static_decl_var"

// Verify that even when a static member declaration is created lazily when
// creating the definition, the declaration line is that of the canonical
// declaration, not the definition. Also, since we look at the canonical
// definition, we should also correctly emit the constant value (42) into the
// debug info.
struct V {
  virtual ~V(); // cause the definition of 'V' to be omitted by no-standalone-debug optimization
  static const int const_va = 42;
};
// CHECK: !MDDerivedType(tag: DW_TAG_member, name: "const_va",
// CHECK-SAME:           line: [[@LINE-3]]
// CHECK-SAME:           extraData: i32 42
const int V::const_va;

namespace x {
struct y {
  static int z;
};
int y::z;
}

// CHECK: !MDGlobalVariable(name: "z",
// CHECK-SAME:              scope: [[NS_X]]
