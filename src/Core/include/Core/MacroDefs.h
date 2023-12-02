#pragma once

/**
 * @file Core/MacroDefs.h contains macro definitions for compiler specific or
 * platform specific constructs
 */

/**
 * @code CGE_unreachable(): declares that a point in the code is never reached,
 * therefore the compiler doesn't have to account for it
 */
#if defined(__GNUC__) || defined(__clang__) // GCC, Clang, ICC
#define GGE_unreachable() (__builtin_unreachable())
#elif defined(_MSC_VER) // MSVC

#define CGE_unreachable() (__assume(false))
#else
// Even if no extension is used, undefined behavior is still raised by
// the empty function body and the noreturn attribute.

// The external definition of unreachable_impl must be emitted in a separated TU
// due to the rule for inline functions in C.

[[noreturn]] inline void unreachableImpl() {}
#define CGE_unreachable() (unreachableImpl())
#endif


/**
 * @code CGE_forceinline: function attribute which instructs the compiler to
 * inline the function in all function calls. Must be defined in-place (same
 * translation unit)
 */
#if defined(__GNUC__) || defined(__clang__) // GCC, Clang, ICC
#define CGE_forceinline __attribute__((always_inline))
#elif defined(_MSC_VER) // MSVC
#define CGE_forceinline __forceinline
#else
#define CGE_forceinline
#endif

/**
 * @code CGE_noinline: function attribute which instructs the compiler to never
 * inline the fuction in all function calls.
 */
#if defined(__GNUC__) || defined(__clang__) // GCC, Clang, ICC
#define CGE_noinline __attribute__((noinline))
#elif defined(_MSC_VER) // MSVC
#define CGE_noinline __declspec(noinline)
#else
#define CGE_forceinline
#endif

/**
 * @code CGE_notnull: function attribute which hints to the compiler the result
 * will never be null (if pointer)
 */
#if defined(__GNUC__) || defined(__clang__)
#define CGE_notnull __attribute__((nonull))
#elif defined(_MSC_VER) // MSVC
#define CGE_notnull _Ret_notnull_
#else
#define CGE_notnull
#endif

/**
 * @code CGE_pure: function attribute. A function is pure if it doesn't modify
 * global memory (but can read it)
 */
#if defined(__GNUC__) || defined(__clang__)
#define CGE_pure __attribute((__pure__))
#elif defined(_MSC_VER)
#define CGE_pure
#else
#define CGE_pure
#endif

/**
 * @code CGE_const: function attribute. A function is const if it doesn't
 * read/write global memory
 */
#if defined(__GNUC__) || defined(__clang__)
#define CGE_const __attribute((__const__))
#elif defined(_MSC_VER)
#define CGE_const
#else
#define CGE_const
#endif

/**
 * @code CGE_restrict: type qualifier for pointers. Hints to the compiler that
 * no other pointer will be used to access the object pointed by the restricted
 * pointer. It enables optimizations
 */
#if defined(__GNUC__) || defined(__clang__)
#define CGE_restrict __restrict__
#elif defined(_MSC_VER)
#define CGE_restrict __restrict
#else
#define CGE_restrict
#endif

/**
 * @code GCE_API: dllimport and dllexport attributes
 * DLL import and export storage class specifier. Shouldn't be needed as long as
 * cge is built as static library
 */
#if defined(__GNUC__) || defined(__clang__)

#if defined(CGE_EXPORT)
#define CGE_API __attribute__(dllexport)
#else
#define CGE_API __attribute__(dllimport)
#endif

#elif defined(_MSC_VER)

#if defined(CGE_EXPORT)
#define CGE_API __declspec(dllexport)
#else
#define CGE_API __declspec(dllimport)
#endif

#else
#define CGE_API
#endif

#if defined(__GNUC__)
#define CGE_vectorcall
#elif defined(_MSC_VER) || defined(__clang__)
#define CGE_vectorcall __vectorcall
#else
#define CGE_vectorcall
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define CGE_PLATFORM_WINDOWS
#elif defined(__linux__) && !defined(__ANDROID__)
#define CGE_PLATFORM_LINUX
#else
#error "Only platform supported are windows and linux"
#endif

#if defined(_DEBUG)
#define CGE_DEBUG
#endif
