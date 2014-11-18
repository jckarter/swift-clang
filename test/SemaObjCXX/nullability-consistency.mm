// RUN: %clang_cc1 -fsyntax-only -fblocks -I %S/Inputs %s -verify

#include "nullability-consistency-1.h"
#include "nullability-consistency-3.h"
#include "nullability-consistency-4.h"

void h1(int *ptr) { } // don't warn

void h2(__nonnull int *) { }

