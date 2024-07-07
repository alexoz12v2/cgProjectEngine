#pragma once

#include "Core/MacroDefs.h"
#include "Core/Type.h"

#if defined(CGE_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(CGE_PLATFORM_LINUX)
#include <ctime>
#include <bits/time.h>
#else
#error "platform not supported"
#endif

namespace cge
{


/**
 * @fn hiResTimer.
 * @brief gets an unsigned integer by reading the time-stamp counter by using,
 * in x86 platforms, the rdtsc instruction. Its value depends on the current
 * number of counter increments per second of the given CPU,
 *        @ref hiResFrequency
 */
inline U64_t hiResTimer()
{
#if defined(CGE_PLATFORM_WINDOWS)
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return static_cast<U64_t>(li.QuadPart);
#elif defined(CGE_PLATFORM_LINUX)
    U32_t low = 0;
    U32_t high = 0;

#if defined(__x86_64__) || defined(__i386__)
    // Use inline assembly for x86 platforms
    asm volatile("rdtsc" : "=a"(low), "=d"(high));
#else
#error "Unsupported architecture"
#endif

    // Combine the low and high parts to get the 64-bit result
    return static_cast<U64_t>(low) | (static_cast<U64_t>(high) << 32);
#endif

    CGE_unreachable();
}

/**
 * @fn hiResFrequency.
 * @brief gets an unsigned integer which corresponts to the number of increments
 * of the time-stamp counter given by the current hardware in a single second
 */
inline U64_t hiResFrequency()
{
#if defined(CGE_PLATFORM_WINDOWS)
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return static_cast<U64_t>(li.QuadPart);
#elif defined(CGE_PLATFORM_LINUX)
    struct timespec res{};
    clock_getres(CLOCK_MONOTONIC, &res);

    // Convert to frequency (1 / delta time)
    return static_cast<U64_t>(1e9 / (double)res.tv_nsec);
#endif

    CGE_unreachable();
}

inline U32_t constexpr timeUnit32 = 3000U;
inline U64_t constexpr timeUnit64 = 3000ULL;
inline F32_t constexpr oneOver60FPS =
  0.0166666693985462188720703125F; // in hex 0x3c88888a;
inline U32_t constexpr timeUnitsIn60FPS = timeUnit32 / 60U; // 50

/**
 * @fn elaspedTimeUnits.
 * @brief returns the elapsed time between the two given time points, in units
 * of 1/300 seconds given the fixed precision the simulation can be more stable,
 * and the time required to make it overflow is 165 years
 */
inline CGE_forceinline U32_t elapsedTimeUnits(U64_t end, U64_t start)
{
    // Convert nanoseconds to 1/300 seconds
    return static_cast<U32_t>((end - start) * timeUnit64 / hiResFrequency());
}

/**
 * @fn elapsedTimeFloat.
 * @brief returns the elapsed time between two given time points, as a binary32
 * floating point. useful for general purpose entity processing in high
 * abstraction level code
 */
inline CGE_forceinline F32_t elapsedTimeFloat(U64_t end, U64_t start)
{
    return (F32_t) (end - start) / (F32_t) hiResFrequency();
}
} // namespace cge
