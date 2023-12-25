#pragma once

#include "Core/Containers.h"
#include "Core/Type.h"

#include <forward_list>
#include <glm/glm.hpp>
#include <map>
#include <memory_resource>
#include <tl/optional.hpp>
#include <vector>

namespace cge
{

struct Vertex_t
{
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec3 texCoords;
};

// necessary to use offsetof macro
static_assert(std::is_standard_layout_v<Vertex_t>);

struct Mesh_s
{
    std::vector<Vertex_t>        vertices;
    std::vector<Array<U32_t, 3>> indices;

    // TODO: reference to textures and/or material
};

AABB_t computeAABB(const Mesh_s& mesh);

} // namespace cge
