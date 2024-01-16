#ifndef CGE_WORLDSPAWNER_H
#define CGE_WORLDSPAWNER_H

#include "Render/VoxelTerrain.h"
#include "Resource/Rendering/GpuProgram.h"
#include "Resource/Rendering/cgeTexture.h"

namespace cge
{

struct Camera_t;

class WorldSpawner
{
  public:
    void init();
    void renderTerrain(Camera_t const &camera);
    void renderBackground(Camera_t const &camera) const;
    void onFramebufferSize(U32_t width, U32_t height);
    void onKey(I32_t key, I32_t action, F32_t deltaTime);

  private:
    void  generateBackgroundCubemap();
    F32_t aspectRatio() const;
    struct Extent2D
    {
        U32_t x;
        U32_t y;
    };

    Extent2D framebufferSize;

    Texture_s    cubeBackground;
    GpuProgram_s backgrProgram;

    VoxelMesh_s terrain{MarchingCubesSpecs_t{}};
    glm::mat4   terrainTransform{ 1.f };

    F32_t clipDistance   = 0.05f;
    F32_t renderDistance = 200.f;
};

} // namespace cge

#endif // CGE_WORLDSPAWNER_H
