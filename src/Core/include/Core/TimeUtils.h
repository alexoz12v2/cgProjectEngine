#pragma once

#include "Core/Type.h"

#if defined(CGE_PLATFORM_WINDOWS)
#include <windows.h>
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
#endif

    CGE_unreachable();
}

inline U32_t constexpr timeUnit32 = 300u;
inline U64_t constexpr timeUnit64 = 300ULL;
inline F32_t constexpr oneOver60FPS =
  0.0166666693985462188720703125f; // in hex 0x3c88888a;
inline U32_t constexpr timeUnitsIn60FPS = timeUnit32 / 60u; // 5

/**
 * @fn elaspedTimeUnits.
 * @brief returns the elapsed time between the two given time points, in units
 * of 1/300 seconds given the fixed precision the simulation can be more stable,
 * and the time required to make it overflow is 165 years
 */
inline CGE_forceinline U32_t elapsedTimeUnits(U64_t end, U64_t start)
{
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
    return static_cast<F32_t>((end - start) / hiResFrequency());
}
} // namespace cge
