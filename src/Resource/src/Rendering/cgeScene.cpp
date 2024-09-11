#include "Rendering/cgeScene.h"

#include <cassert>
#include <glm/ext/matrix_transform.hpp>

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

glm::vec4 const &SceneNode_s::getPosition() const
{ //
    return m_transform[3];
}

void SceneNode_s::setSid(Sid_t sid)
{ //
    m_sid = sid;
}

void SceneNode_s::transform(glm::mat4 const &t)
{
    m_transform = t * m_transform;
}

void SceneNode_s::rightMul(glm::mat4 const &t) {
    m_transform *= t;
}

void SceneNode_s::setTransform(const glm::mat4 &t)
{
    m_transform = t;
}
void SceneNode_s::translate(const glm::vec3 &disp)
{
    m_transform[3].x += disp.x;
    m_transform[3].y += disp.y;
    m_transform[3].z += disp.z;
}

void SceneNode_s::rotate(F32_t radians, glm::vec3 const &rotationAxis)
{
    m_transform = glm::rotate(m_transform, radians, rotationAxis);
}

SceneNode_s &Scene_s::getNodeBySid(Sid_t sid)
{
    return m_nodeMap.at(sid);
}

SceneNode_s const &Scene_s::getNodeBySid(Sid_t sid) const
{
    return m_nodeMap.at(sid);
}

Scene_s::PairNode const &Scene_s::getNodePairByNodeRef(SceneNode_s const &ref) const
{
    auto const it = std::ranges::find_if(
      m_nodeMap,
      [ref](std::pair<Sid_t const, SceneNode_s> const &p) -> B8_t
      { return p.second.getSid() == ref.getSid() && p.second.getTransform() == ref.getTransform(); });
    if (it == m_nodeMap.cend()) { assert(false && "[Scene] searching for inexistent nodes is illegal"); }

    return *it;
}

Scene_s::PairNode &Scene_s::getNodePairByNodeRef(SceneNode_s const &ref)
{
    auto const it = std::ranges::find_if(
      m_nodeMap,
      [ref](std::pair<Sid_t const, SceneNode_s> const &p) -> B8_t
      { return p.second.getSid() == ref.getSid() && p.second.getTransform() == ref.getTransform(); });
    if (it == m_nodeMap.cend()) { assert(false && "[Scene] searching for inexistent nodes is illegal"); }

    return *it;
}

Sid_t Scene_s::addNode(Sid_t const meshSid)
{
    Sid_t sceneSid = meshSid;
    auto  p        = m_nodeMap.try_emplace(sceneSid, SceneNode_s(meshSid));
    while (!p.second)
    {
        ++sceneSid.id;
        p = m_nodeMap.try_emplace(sceneSid, SceneNode_s(meshSid));
    }

    return p.first->first;
}

B8_t Scene_s::removeNode(Sid_t nodeSid)
{ //
    return m_nodeMap.erase(nodeSid) > 0;
}

void Scene_s::clearSceneNodes()
{ //
    m_nodeMap.clear();
}
Scene_s::LightConstIt Scene_s::lightBegin() const
{ //
    return m_lightMap.cbegin();
}

Scene_s::LightIt Scene_s::lightBegin()
{ //
    return m_lightMap.begin();
}

Scene_s::LightConstIt Scene_s::lightEnd() const
{ //
    return m_lightMap.cend();
}

Scene_s::LightIt Scene_s::lightEnd()
{ //
    return m_lightMap.end();
}

Sid_t Scene_s::addLight(Sid_t lightSid, Light_t const &light)
{
    Sid_t sceneSid = lightSid;
    auto  p        = m_lightMap.try_emplace(sceneSid, light);
    while (!p.second)
    {
        ++sceneSid.id;
        p = m_lightMap.try_emplace(sceneSid, light);
    }

    return p.first->first;
}

B8_t Scene_s::removeLight(Sid_t lightSid)
{ //
    return m_lightMap.erase(lightSid) > 0;
}

void Scene_s::clearSceneLights()
{ //
    m_lightMap.clear();
}

} // namespace cge
