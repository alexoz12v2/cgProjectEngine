#include "Random.h"

namespace cge
{
// Static mutex initialization (only once)
thread_local Random g_random;

std::mt19937 &Random::getGen()
{ //
    return m_generator;
}
} // namespace cge
