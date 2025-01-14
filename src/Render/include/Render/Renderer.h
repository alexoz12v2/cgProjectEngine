#pragma once

#include "Core/Containers.h"
#include "Core/Type.h"
#include "Resource/Rendering/GpuProgram.h"
#include "Resource/Rendering/cgeScene.h"
#include "Resource/Rendering/cgeTexture.h"

#include <glm/glm.hpp>

namespace cge
{

struct Camera_t
{
    glm::mat4 viewTransform() const;
    void      setForward(glm::vec3 newForward);

    glm::vec3 forward;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 position;
};

class Renderer_s
{
  public:
    void init();
    void renderScene(
      Scene_s const   &scene,
      glm::mat4 const &view,
      glm::mat4 const &proj,
      glm::vec3        eye) const;
    void clear() const;

    void renderCube() const;

    void onFramebufferSize(I32_t width, I32_t height);

  private:
    U32_t m_width;
    U32_t m_height;
};

extern Renderer_s g_renderer;

class BackgroundRenderer
{
  public:
    void init(unsigned char *image, I32_t width, I32_t height);
    void renderBackground(
      Camera_t const &camera,
      glm::mat4 const &proj) const;

  private:
    Texture_s    m_cubeBackground;
    GpuProgram_s m_backgrProgram;
    B8_t         m_init{ false };
};

BackgroundRenderer &getBackgroundRenderer();

} // namespace cge
