#pragma once

#include "Core/StringUtils.h"
#include "Core/Type.h"

#include <glm/glm.hpp>
#include <tl/optional.hpp>

#include <forward_list>
#include <map>
#include <vector>
#include <string>

extern "C"
{
    struct aiNode;
    struct aiScene;
}

namespace cge
{

struct SceneNode_s
{
    Sid_t        sid;               // sid of the mesh
    glm::mat4    absoluteTransform; // Absolute transform
    glm::mat4    relativeTransform;
    SceneNode_s *parent; // Pointer to the parent node
    B8_t         translucent;

    std::pmr::forward_list<SceneNode_s *> children;
};

class Scene_s
{
    friend class Renderer_s;

  public:
    static tl::optional<Scene_s> fromObj(Char8_t const *path);

    SceneNode_s *getNodeBySid(Sid_t sid)
    {
        auto it = m_bnodes.find(sid);
        return (it != m_bnodes.end()) ? &(it->second) : nullptr;
    }

    B8_t addChild(Sid_t parent, Sid_t child);

    B8_t removeChild(Sid_t parent, Sid_t child);

    B8_t removeNode(Sid_t sid);

    std::pmr::vector<Sid_t>::const_iterator names() const
    {
        return m_names.cbegin();
    }

    std::pmr::vector<Sid_t>::const_iterator namesEnd() const
    {
        return m_names.cend();
    }

  private:
    void processNode(
      Sid_t          parent,
      aiNode const  *node,
      aiScene const *aScene,
      Scene_s       *outScene);

    // trasform is relative to parent
    SceneNode_s &
      createNode(Sid_t sid, glm::mat4 const &transform = glm::mat4(1.f));

    std::pmr::map<Sid_t, SceneNode_s> m_bnodes;
    std::pmr::vector<Sid_t>           m_names;
    std::string                       m_path;
};
extern Scene_s g_scene;

} // namespace cge
