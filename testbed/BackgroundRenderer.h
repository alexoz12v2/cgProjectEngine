#pragma once

#include "Render/Renderer.h"
#include "Resource/Rendering/GpuProgram.h"
#include "Resource/Rendering/ShaderLibrary.h"
#include "Resource/Rendering/cgeTexture.h"

namespace cge
{

class BackgroundRenderer
{ //
  public:
    void init();
    void renderBackground(Camera_t const &camera, F32_t aspectRatio) const;

  private:
    Texture_s    m_cubeBackground;
    GpuProgram_s m_backgrProgram;
};

} // namespace cge
