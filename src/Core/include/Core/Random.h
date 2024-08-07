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
    Random()
    {
        std::random_device rd;
        m_generator.seed(rd());
    }

    // Generates a random number within the specified range (inclusive)
    template<std::integral T>
    T next(
      T min = std::numeric_limits<T>::min(),
      T max = std::numeric_limits<T>::max())
    {
        std::uniform_int_distribution<T> distribution(min, max);
        return distribution(m_generator);
    }

    // Generates a random double between 0.0 (inclusive) and 1.0 (exclusive)
    template<std::floating_point T> T next()
    {
        std::uniform_real_distribution<T> distribution(0.0, 1.0);
        return distribution(m_generator);
    }

    std::mt19937 &getGen();

  private:
    // Thread-local random number m_generator
    std::mt19937 m_generator;

    // Disallow copying to prevent accidental sharing of the m_generator
    Random(const Random &)            = delete;
    Random &operator=(const Random &) = delete;
};

extern thread_local Random g_random;

} // namespace cge
