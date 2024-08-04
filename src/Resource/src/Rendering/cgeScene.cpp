#include "Rendering/cgeScene.h"

#include <cassert>

namespace cge
{
Scene_s g_scene;

SceneNode_s::SceneNode_s() : m_sid(nullSid) {}

SceneNode_s::SceneNode_s(Sid_t sid) : m_sid(sid) {}

Sid_t SceneNode_s::getSid() const
{ //
    return m_sid;
}

glm::mat4 const &SceneNode_s::getTransform() const
{ //
    return m_transform;
}

void SceneNode_s::setSid(Sid_t sid)
{ //
    m_sid = sid;
}

void SceneNode_s::transform(glm::mat4 const &t)
{
    m_transform = t * m_transform;
}

SceneNode_s &Scene_s::getNodeBySid(Sid_t sid)
{ //
    return m_nodeMap.at(sid);
}

SceneNode_s const &Scene_s::getNodeBySid(Sid_t sid) const
{ //
    return m_nodeMap.at(sid);
}

Scene_s::PairNode const &
  Scene_s::getNodePairByNodeRef(SceneNode_s const &ref) const
{
    auto const it = std::ranges::find_if(
      m_nodeMap,
      [ref](std::pair<Sid_t const, SceneNode_s> const &p) -> B8_t
      {
          return p.second.getSid() == ref.getSid()
                 && p.second.getTransform() == ref.getTransform();
      });
    if (it == m_nodeMap.cend())
    {
        assert(false && "[Scene] searching for inexistent nodes is illegal");
    }

    return *it;
}

Scene_s::PairNode &Scene_s::getNodePairByNodeRef(SceneNode_s const &ref)
{
    auto const it = std::ranges::find_if(
      m_nodeMap,
      [ref](std::pair<Sid_t const, SceneNode_s> const &p) -> B8_t
      {
          return p.second.getSid() == ref.getSid()
                 && p.second.getTransform() == ref.getTransform();
      });
    if (it == m_nodeMap.cend())
    {
        assert(false && "[Scene] searching for inexistent nodes is illegal");
    }

    return *it;
}

std::pair<Sid_t const, SceneNode_s> &Scene_s::addNode(Sid_t const meshSid)
{
    Sid_t sceneSid = meshSid;
    auto  p        = m_nodeMap.try_emplace(sceneSid, SceneNode_s(meshSid));
    while (!p.second)
    {
        ++sceneSid.id;
        p = m_nodeMap.try_emplace(sceneSid, SceneNode_s(meshSid));
    }

    return *p.first;
}

B8_t Scene_s::removeNode(Sid_t nodeSid)
{ //
    return m_nodeMap.erase(nodeSid) > 0;
}

void Scene_s::clear()
{ //
    m_nodeMap.clear();
}

} // namespace cge
