#ifndef CGE_CONSTANTSANDSTRUCTS_H
#define CGE_CONSTANTSANDSTRUCTS_H

#include <glm/ext/vector_float3.hpp>

namespace cge
{

struct HitInfo_t
{
    alignas(float) bool present; // std430
    glm::vec3 position;
    glm::vec3 normal;
};

struct Extent2D
{
    unsigned x;
    unsigned y;
};

inline F32_t constexpr CLIPDISTANCE   = 0.05F;
inline F32_t constexpr RENDERDISTANCE = 200.F;
inline F32_t constexpr FOV = 45.F;

} // namespace cge

#endif // CGE_CONSTANTSANDSTRUCTS_H
