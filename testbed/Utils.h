#pragma once

#include "Core/Type.h"
#include "Render/Renderer.h"

#include <glm/common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

#undef min
#undef max

namespace cge
{

inline bool smallOrZero(glm::vec3 v)
{
    static F32_t constexpr eps = std::numeric_limits<F32_t>::epsilon();
    v                          = glm::abs(v);
    return v.x <= eps && v.y <= eps && v.z <= eps;
}

inline AABB globalSpaceBB(SceneNode_s const &ptr, AABB aabb)
{
    glm::mat4 modelMatrix = ptr.getTransform();

    // Extract corners of the AABB
    glm::vec3 min = aabb.mm.min;
    glm::vec3 max = aabb.mm.max;

    glm::vec3 corners[8] = { glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z),
                             glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z),
                             glm::vec3(min.x, min.y, max.z), glm::vec3(max.x, min.y, max.z),
                             glm::vec3(min.x, max.y, max.z), glm::vec3(max.x, max.y, max.z) };

    // Transform each corner by the model matrix
    glm::vec3 transformedCorners[8];
    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 transformedCorner = modelMatrix * glm::vec4(corners[i], 1.0f);
        transformedCorners[i]       = glm::vec3(transformedCorner);
    }

    // Compute the min and max of the transformed AABB
    glm::vec3 newMin = transformedCorners[0];
    glm::vec3 newMax = transformedCorners[0];
    for (int i = 1; i < 8; ++i)
    {
        newMin = glm::min(newMin, transformedCorners[i]);
        newMax = glm::max(newMax, transformedCorners[i]);
    }

    return AABB(newMin, newMax);
}

inline AABB globalSpaceBB(Sid_t sid, AABB aabb)
{
    return globalSpaceBB(g_scene.getNodeBySid(sid), aabb);
}

} // namespace cge
