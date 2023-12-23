#pragma once

#include "Core/Containers.h"
#include "Core/Type.h"

#include <glm/glm.hpp>
#include <tl/optional.hpp>
#include <vector>

extern "C"
{
    struct aiScene;
}

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

// TODO better. Remove vector and put Refs
struct Mesh_s
{
    std::vector<Vertex_t>        vertices;
    std::vector<Array<U32_t, 3>> indices;
    glm::mat4                    transform;
};

struct MeshSpec_t
{
    struct aiScene const *pScene;
};
class Scene_s
{
  public:
    static tl::optional<Scene_s> open(Char8_t const *path);
    Mesh_s                       getMesh() const;

    Scene_s() = delete;
    ~Scene_s();
    explicit Scene_s(MeshSpec_t const &spec);

  private:
    aiScene const *m_scene;
};

} // namespace cge
