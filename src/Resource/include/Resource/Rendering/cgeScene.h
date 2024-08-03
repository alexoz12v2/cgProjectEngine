#pragma once

#include "Core/Module.h"
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

class SceneNode_s;
class Scene_s;

class SceneNodeHandle
{
    friend class Scene_s;

  private:
    constexpr SceneNodeHandle(Sid_t sid, Scene_s *pScene) noexcept;

  public:
    SceneNode_s const &operator*() const;
    SceneNode_s       &operator*();

    SceneNode_s const *operator->() const;
    SceneNode_s       *operator->();

    Sid_t getSid() const;

    B8_t isValid() const;

  private:
    Sid_t    m_sid;
    Scene_s *m_pScene;
};

class SceneNode_s
{
    friend class Scene_s;

  private:
    struct Tag
    {
    };
    static Tag constexpr tag{};

  public:
    using ChildList = std::pmr::vector<SceneNodeHandle>;

  public:
    SceneNode_s(Sid_t sid_, SceneNodeHandle parent_);

  public:
    Sid_t     getSid() const { return m_sid; }
    glm::mat4 getRelativeTransform() const { return m_relativeTransform; }
    glm::mat4 getAbsoluteTransform() const;

    void setSid(Sid_t sid) { m_sid = sid; }
    void transform(glm::mat4 const &t);

  private:
    SceneNode_s(Sid_t sid_, Tag);

    Sid_t                         m_sid; // sid of the mesh
    tl::optional<SceneNodeHandle> m_parent;
    ChildList m_children{ getMemoryPool() }; // stored in bnodes
    glm::mat4 m_relativeTransform = glm::mat4(1.f);
    B8_t      m_translucent       = false;
};

class Scene_s
{
    friend class Renderer_s;
    friend class SceneNodeHandle;

  public:
    SceneNodeHandle getNodeBySid(Sid_t sid);

    SceneNodeHandle addChild(
      SceneNodeHandle parent,
      Sid_t           childSid,
      glm::mat4       transform = glm::mat4(1.f));
    SceneNodeHandle
      addChild(Sid_t childSid, glm::mat4 transform = glm::mat4(1.f));

    B8_t removeChild(SceneNodeHandle parent, Sid_t child);
    B8_t removeNode(SceneNodeHandle node);

    std::pmr::vector<Sid_t>::const_iterator names() const;
    std::pmr::vector<Sid_t>::const_iterator namesEnd() const;

    SceneNode_s &getRoot() { return m_root; }

  private:
    // trasform is relative to parent
    SceneNodeHandle createNode(
      Sid_t            sid,
      SceneNodeHandle  parent,
      glm::mat4 const &transform = glm::mat4(1.f));

  private:
    std::pmr::map<Sid_t, SceneNode_s> m_bnodes{ getMemoryPool() };
    SceneNode_s             m_root = SceneNode_s(nullSid, SceneNode_s::tag);
    std::pmr::vector<Sid_t> m_names{ getMemoryPool() };
};

extern Scene_s g_scene;

} // namespace cge
