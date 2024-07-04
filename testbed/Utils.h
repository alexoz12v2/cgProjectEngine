#pragma once

#include "Core/Type.h"
#include "Render/Renderer.h"

#include <glm/common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

namespace cge
{

inline bool smallOrZero(glm::vec3 v)
{
    static F32_t constexpr eps = std::numeric_limits<F32_t>::epsilon();
    v                          = glm::abs(v);
    return v.x <= eps && v.y <= eps && v.z <= eps;
}

AABB_t recomputeGlobalSpaceBB(Sid_t sid, AABB_t box)
{
    glm::mat4 transform = g_scene.getNodeBySid(sid)->getAbsoluteTransform();

    AABB_t const gSpaceAABB = { .min = transform * glm::vec4(box.min, 1.f),
                                .max = transform * glm::vec4(box.max, 1.f) };

    return gSpaceAABB;
}

} // namespace cge
