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

} // namespace cge
