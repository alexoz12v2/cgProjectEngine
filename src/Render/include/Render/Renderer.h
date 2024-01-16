#pragma once

#include "Core/Containers.h"
#include "Core/Type.h"

#include <Resource/Rendering/cgeScene.h>
#include <glm/glm.hpp>

namespace cge
{

struct Camera_t
{
    glm::mat4 viewTransform() const;

    glm::vec3 forward;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 position;
};

class Renderer_s
{
  public:
    void renderScene(
      Scene_s const   &scene,
      glm::mat4 const &view,
      glm::mat4 const &proj) const;
    void clear() const;

    // TODO configuration program
    void renderCube() const;

    void viewport(U32_t width, U32_t height) const;
};

extern Renderer_s g_renderer;

} // namespace cge
