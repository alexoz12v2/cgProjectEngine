#include "Random.h"

namespace cge
{
// Static mutex initialization (only once)
std::mutex Random::mtx;
thread_local Random g_random;
}
