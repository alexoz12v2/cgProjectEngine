#pragma once
#include "Type.h"

#include <concepts>
#include <mutex>
#include <random>
#include <thread>

namespace cge
{

class Random
{
  public:
    // Generates a random number within the specified range (inclusive)
    template<std::integral T>
    T next(
      T min = std::numeric_limits<T>::min(),
      T max = std::numeric_limits<T>::max())
    {
        std::uniform_int_distribution<T> distribution(min, max);
        return distribution(generator);
    }

    // Generates a random double between 0.0 (inclusive) and 1.0 (exclusive)
    template<std::floating_point T> T next()
    {
        std::uniform_real_distribution<T> distribution(0.0, 1.0);
        return distribution(generator);
    }

  private:
    // Thread-local random number generator
    std::mt19937 generator;

    // Mutex for thread-safe initialization
    static std::mutex mtx;

    Random()
    {
        // Seed the generator with a thread-safe random device
        std::random_device          rd;
        std::lock_guard<std::mutex> lock(mtx);
        generator.seed(rd());
    }

    // Disallow copying to prevent accidental sharing of the generator
    Random(const Random&)            = delete;
    Random& operator=(const Random&) = delete;
};

extern thread_local Random g_random;

} // namespace cge
