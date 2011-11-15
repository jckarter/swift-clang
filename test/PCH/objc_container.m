// Test this without pch.
// RUN: %clang_cc1 -include %S/objc_container.h -fsyntax-only -verify %s

// Test with pch.
// RUN: %clang_cc1 -x objective-c -emit-pch -o %t %S/objc_container.h
// RUN: %clang_cc1 -include-pch %t -fsyntax-only -verify %s 

int main() {
  NSMutableArray *array;
  id oldObject = array[10];

  array[10] = oldObject;

  NSMutableDictionary *dictionary;
  id key;
  id newObject;
  oldObject = dictionary[key];

  dictionary[key] = newObject;
}

