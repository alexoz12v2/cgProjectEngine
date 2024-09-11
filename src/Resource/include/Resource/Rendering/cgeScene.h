#pragma once

#include "Core/Module.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "cgeLight.h"

#include <glm/glm.hpp>

#include <unordered_map>

namespace cge
{

class SceneNode_s;
class Scene_s;

class SceneNode_s
{
    friend class Scene_s;

  public:
    SceneNode_s();
    explicit SceneNode_s(Sid_t sid);

  public:
    Sid_t            getSid() const;
    glm::mat4 const &getTransform() const;
    glm::vec4 const &getPosition() const;

    void setSid(Sid_t sid);
    void transform(glm::mat4 const &t);
    void rightMul(glm::mat4 const &t);
    void setTransform(glm::mat4 const &t);
    void translate(glm::vec3 const &disp);
    void rotate(F32_t radians, glm::vec3 const &rotationAxis);

  private:
    Sid_t     m_sid; // sid of the mesh
    glm::mat4 m_transform = glm::mat4(1.f);
};

class Scene_s
{
    friend class Renderer_s;

  public:
    using PairNode     = std::pair<Sid_t const, SceneNode_s>;
    using LightIt      = std::pmr::unordered_map<Sid_t, Light_t>::iterator;
    using LightConstIt = std::pmr::unordered_map<Sid_t, Light_t>::const_iterator;

  public:
    SceneNode_s       &getNodeBySid(Sid_t sid);
    SceneNode_s const &getNodeBySid(Sid_t sid) const;
    PairNode const    &getNodePairByNodeRef(SceneNode_s const &ref) const;
    PairNode          &getNodePairByNodeRef(SceneNode_s const &ref);
    LightConstIt       lightBegin() const;
    LightIt            lightBegin();
    LightConstIt       lightEnd() const;
    LightIt            lightEnd();

    Sid_t addNode(Sid_t const meshSid);
    B8_t  removeNode(Sid_t node);
    void  clearSceneNodes();
    Sid_t addLight(Sid_t lightSid, Light_t const &light);
    B8_t  removeLight(Sid_t lightSid);
    void  clearSceneLights();

  private:
    std::pmr::unordered_map<Sid_t, SceneNode_s> m_nodeMap{ getMemoryPool() };
    std::pmr::unordered_map<Sid_t, Light_t>     m_lightMap{ getMemoryPool() };
};

extern Scene_s g_scene;

} // namespace cge
