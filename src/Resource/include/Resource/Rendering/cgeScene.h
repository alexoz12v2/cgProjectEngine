#pragma once

#include "Core/StringUtils.h"
#include "Core/Type.h"

#include <glm/glm.hpp>
#include <tl/optional.hpp>

#include <forward_list>
#include <map>
#include <string>
#include <vector>

extern "C"
{
    struct aiNode;
    struct aiScene;
}

namespace cge
{


class Scene_s;
struct SceneNode_s
{
    friend class Scene_s;
    using ChildList = std::pmr::vector<SceneNode_s *>;

    SceneNode_s(Sid_t sid_, SceneNode_s &parent_)
      : m_sid(sid_), m_isRoot(false), m_parent(parent_)
    {
    }

    Sid_t        getSid() const { return m_sid; }
    glm::mat4    getRelativeTransform() const { return m_relativeTransform; }
    glm::mat4    getAbsoluteTransform() const;
    SceneNode_s *getParent() const
    {
        if (m_isRoot) { return &m_parent.parent.get(); }
        else { return nullptr; }
    }

    void setSid(Sid_t sid) { m_sid = sid; }

    void transform(glm::mat4 const &t)
    {
        m_relativeTransform = t * m_relativeTransform;
    }

  private:
    SceneNode_s(Sid_t sid_) : m_sid(sid_), m_isRoot(true), m_parent(true) {}

    union U
    {
        U(SceneNode_s &x) : parent(x) {}
        U(B8_t x) : isRoot(x) {}
        std::reference_wrapper<SceneNode_s> parent;
        B8_t                                isRoot;
    };

    Sid_t     m_sid; // sid of the mesh
    U         m_parent;
    B8_t      m_isRoot;
    ChildList m_children          = ChildList(); // stored in bnodes
    glm::mat4 m_relativeTransform = glm::mat4(1.f);
    B8_t      m_translucent       = false;
};

class Scene_s
{
    friend class Renderer_s;

  public:
    Scene_s()                = default;
    Scene_s(Scene_s const &) = default;
    static Scene_s fromObj(Char8_t const *path);

    void mergeWith(Scene_s const &other);

    SceneNode_s *getNodeBySid(Sid_t sid)
    {
        auto it = m_bnodes.find(sid);
        return (it != m_bnodes.cend()) ? &(it->second) : nullptr;
    }

    SceneNode_s const *getNodeBySid(Sid_t sid) const
    {
        auto it = m_bnodes.find(sid);
        return (it != m_bnodes.cend()) ? &(it->second) : nullptr;
    }

    auto getAllNodesBySid(Sid_t sid) { return m_bnodes.equal_range(sid); }
    auto getAllNodesBySid(Sid_t sid) const { return m_bnodes.equal_range(sid); }

    B8_t isEmptyRange(std::pair<
                      std::pmr::multimap<Sid_t, SceneNode_s>::iterator,
                      std::pmr::multimap<Sid_t, SceneNode_s>::iterator> p) const
    {
        return p.first != m_bnodes.end();
    }

    B8_t isEmptyRange(
      std::pair<
        std::pmr::multimap<Sid_t, SceneNode_s>::const_iterator,
        std::pmr::multimap<Sid_t, SceneNode_s>::const_iterator> p) const
    {
        return p.first != m_bnodes.cend();
    }

    SceneNode_s *addChild(
      SceneNode_s &parent,
      Sid_t        childSid,
      glm::mat4    transform = glm::mat4(1.f));
    SceneNode_s *addChild(Sid_t childSid, glm::mat4 transform = glm::mat4(1.f))
    {
        return addChild(m_root, childSid, transform);
    };

    B8_t removeChild(SceneNode_s *parent, Sid_t child);

    std::pmr::vector<Sid_t>::const_iterator names() const
    {
        return m_names.cbegin();
    }

    std::pmr::vector<Sid_t>::const_iterator namesEnd() const
    {
        return m_names.cend();
    }

    SceneNode_s &getRoot() { return m_root; }

  private:
    void processNode(
      SceneNode_s   &parent,
      aiNode const  *node,
      aiScene const *aScene,
      Char8_t const *path);

    // trasform is relative to parent
    SceneNode_s *createNode(
      Sid_t            sid,
      SceneNode_s     &parent,
      glm::mat4 const &transform = glm::mat4(1.f));

    std::pmr::multimap<Sid_t, SceneNode_s> m_bnodes;
    SceneNode_s                            m_root = SceneNode_s(nullSid);
    std::pmr::vector<Sid_t>                m_names;
};

extern Scene_s g_scene;

} // namespace cge
