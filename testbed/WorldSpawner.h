#ifndef CGE_WORLDSPAWNER_H
#define CGE_WORLDSPAWNER_H

#include "Render/VoxelTerrain.h"
#include "Resource/Rendering/GpuProgram.h"
#include "Resource/Rendering/ShaderLibrary.h"
#include "Resource/Rendering/cgeTexture.h"

#include "ConstantsAndStructs.h"

namespace cge
{

struct Camera_t;

class WorldSpawner
{
  public:
    void init();
    void renderTerrain(Camera_t const &camera);
    void renderBackground(Camera_t const &camera) const;
    void transformTerrain(glm::mat4 const &transform);
    HitInfo_t detectTerrainCollisions(glm::mat4 const &transform); // TODO add a mesh here

    void onFramebufferSize(U32_t width, U32_t height);
    void onKey(I32_t key, I32_t action);

  private:
    void  generateBackgroundCubemap();
    void createCollisionBuffersAndShaders();
    void bindCollisionbuffers() const;

    F32_t aspectRatio() const;

    Extent2D m_framebufferSize;

    Texture_s    m_cubeBackground;
    GpuProgram_s m_backgrProgram;

    VoxelMesh_s m_terrain{MarchingCubesSpecs_t{}};
    glm::mat4   m_terrainTransform{ 1.f };

    struct CollisionData_t {
        GpuProgram_s buildShader;
        GpuProgram_s detectShader;
        GpuProgram_s resetShader;

        using HashGridBuffer_s = DerivedBuffer_s<GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY>;
        using HitInfoBuffer_s = DerivedBuffer_s<GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY>;
        HashGridBuffer_s hashGridBuffer;
        HitInfoBuffer_s  hitInfoBuffer;
    };
    CollisionData_t m_collision;
};

} // namespace cge

#endif // CGE_WORLDSPAWNER_H
