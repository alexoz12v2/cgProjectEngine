#pragma once
// MIT License. Copyright (c) 2023

/**
 * @file Core/MacroDefs.h contains macro definitions for compiler specific or platform specific
 * constructs
 */
#if defined(__GNUC__) || defined(__clang__)// GCC, Clang, ICC
#define unreachable() (__builtin_unreachable())
#elif defined(_MSC_VER)// MSVC

#define CGE_unreachable() (__assume(false))
#else
// Even if no extension is used, undefined behavior is still raised by
// the empty function body and the noreturn attribute.

// The external definition of unreachable_impl must be emitted in a separated TU
// due to the rule for inline functions in C.

[[noreturn]] inline void unreachableImpl() {}
#define CGE_unreachable() (unreachableImpl())
#endif