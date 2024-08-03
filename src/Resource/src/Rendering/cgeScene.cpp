#include "Rendering/cgeScene.h"

#include "HandleTable.h"
#include "Rendering/cgeMesh.h"

#include "Core/Random.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glad/gl.h>
#include <stb/stb_image.h>

#include <algorithm>
#include <cassert>

namespace cge
{
Scene_s g_scene;

std::pmr::vector<Sid_t>::const_iterator Scene_s::names() const
{
    return m_names.cbegin();
}

std::pmr::vector<Sid_t>::const_iterator Scene_s::namesEnd() const
{
    return m_names.cend();
}

SceneNodeHandle Scene_s::createNode(
  Sid_t            sid,
  SceneNodeHandle  parent,
  glm::mat4 const &transform)
{
    auto s                = SceneNode_s(sid, parent);
    s.m_relativeTransform = transform;
    auto p                = m_bnodes.try_emplace(sid, s);
    while (!p.second)
    { //
        sid.id = g_random.next<U64_t>();
        p      = m_bnodes.try_emplace(sid, s);
    }
    m_names.push_back(sid);
    parent->m_children.push_back(SceneNodeHandle(p.first->first, this));

    return SceneNodeHandle(p.first->first, this);
}

SceneNodeHandle Scene_s::getNodeBySid(Sid_t sid)
{ //
    return SceneNodeHandle(sid, this);
}

SceneNodeHandle
  Scene_s::addChild(SceneNodeHandle parent, Sid_t childSid, glm::mat4 transform)
{
    return createNode(childSid, parent, transform);
}

SceneNodeHandle Scene_s::addChild(Sid_t childSid, glm::mat4 transform)
{
    return addChild(SceneNodeHandle(nullSid, this), childSid, transform);
};

B8_t Scene_s::removeNode(SceneNodeHandle node)
{
    if (node.isValid())
    {
        tl::optional<SceneNodeHandle> parentOpt = node->m_parent;
        Sid_t const                   sid       = node->m_sid;

        if (parentOpt.has_value())
        {
            SceneNodeHandle parent = *parentOpt;
            U32_t           idx    = parent->m_children.size();
            for (U32_t i = 0; i != parent->m_children.size(); ++i)
            {
                if (parent->m_children[i].m_sid == node.m_sid)
                {
                    idx = i;
                    break;
                }
            }

            if (idx != parent->m_children.size())
            {
                auto it = parent->m_children.begin() + idx;
                parent->m_children.erase(it);
            }

            m_bnodes.erase(sid);
        }
        return true;
    }
    else
        return false;
}

inline B8_t Scene_s::removeChild(SceneNodeHandle parent, Sid_t child)
{
    auto it = std::remove_if(
      parent->m_children.begin(),
      parent->m_children.end(),
      [child](SceneNodeHandle x) { return x->m_sid == child; });

    if (it != parent->m_children.end())
    {
        parent->m_children.erase(it, parent->m_children.end());
        m_bnodes.erase(child);

        return true;
    }

    return false;
}

glm::mat4 SceneNode_s::getAbsoluteTransform() const
{
    SceneNode_s const *current = this;
    glm::mat4          t       = m_relativeTransform;
    while (!current->m_parent.has_value())
    {
        t = (*current->m_parent)->m_relativeTransform * t;

        current = &*(*current->m_parent);
    }

    return t;
}

SceneNode_s::SceneNode_s(Sid_t sid_, SceneNodeHandle parent_)
  : m_sid(sid_), m_parent(parent_)
{
    m_children.reserve(32);
}

SceneNode_s::SceneNode_s(Sid_t sid_, Tag) : m_sid(sid_), m_parent(tl::nullopt)
{
    m_children.reserve(32);
}

void SceneNode_s::transform(glm::mat4 const &t)
{
    m_relativeTransform = t * m_relativeTransform;
}
constexpr SceneNodeHandle::SceneNodeHandle(Sid_t sid, Scene_s *pScene) noexcept
  : m_sid(sid), m_pScene(pScene)
{
    if (!m_pScene)
    { //
        assert(false && "[Scene] invalid arguments");
    }
}

SceneNode_s const &SceneNodeHandle::operator*() const
{
    if (m_sid == nullSid)
    { //
        return m_pScene->m_root;
    }
    return m_pScene->m_bnodes.at(m_sid);
}

SceneNode_s &SceneNodeHandle::operator*()
{
    if (m_sid == nullSid)
    { //
        return m_pScene->m_root;
    }
    return m_pScene->m_bnodes.at(m_sid);
}

SceneNode_s const *SceneNodeHandle::operator->() const
{
    if (m_sid == nullSid)
    { //
        return &m_pScene->m_root;
    }
    auto it = m_pScene->m_bnodes.find(m_sid);
    if (it == m_pScene->m_bnodes.cend())
    { //
        assert(false);
    }
    return &it->second;
}

SceneNode_s *SceneNodeHandle::operator->()
{
    if (m_sid == nullSid)
    { //
        return &m_pScene->m_root;
    }
    auto it = m_pScene->m_bnodes.find(m_sid);
    if (it == m_pScene->m_bnodes.cend())
    { //
        assert(false);
    }
    return &it->second;
}

Sid_t SceneNodeHandle::getSid() const
{ //
    return m_sid;
}

B8_t SceneNodeHandle::isValid() const
{
    if (m_sid == nullSid)
    { //
        return true;
    }
    auto it = m_pScene->m_bnodes.find(m_sid);
    if (it == m_pScene->m_bnodes.cend())
    { //
        return false;
    }
    return true;
}

} // namespace cge
