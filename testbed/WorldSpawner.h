#ifndef CGE_WORLDSPAWNER_H
#define CGE_WORLDSPAWNER_H

#include "Render/VoxelTerrain.h"
#include "Resource/Rendering/GpuProgram.h"
#include "Resource/Rendering/ShaderLibrary.h"
#include "Resource/Rendering/cgeTexture.h"

#include "ConstantsAndStructs.h"

#include <glm/ext/matrix_transform.hpp>

namespace cge
{

struct Camera_t;

struct Tile
{
    // indices of each cell in the 5x5 grid, in column-major order, of the used
    // mesh in cell
    std::array<U16_t, 25> indices;

    std::array<Sid_t, 25> meshSids;

    glm::mat4 transform;
};

class WorldSpawner
{
  public:
    static U32_t constexpr chunksCount = 2;

    void      init();
    void      renderTerrain(Camera_t const &camera);
    void      renderBackground(Camera_t const &camera) const;
    void      transformTerrain(glm::mat4 const &transform);
    HitInfo_t detectTerrainCollisions(
      glm::mat4 const &transform); // TODO add a mesh here

    void onFramebufferSize(U32_t width, U32_t height);

  private:
    void generateBackgroundCubemap();
    void createCollisionBuffersAndShaders();
    void bindCollisionbuffers() const;

    F32_t aspectRatio() const;

    Extent2D m_framebufferSize;

    Texture_s    m_cubeBackground;
    GpuProgram_s m_backgrProgram;

    std::array<VoxelMesh_s, chunksCount> m_terrain{
        VoxelMesh_s(MarchingCubesSpecs_t{}),
        VoxelMesh_s(MarchingCubesSpecs_t{})
    };
    std::array<glm::mat4, chunksCount> m_terrainTransform{
        glm::mat4{ 1.F },
        glm::translate(glm::mat4(1.F), glm::vec3(10, 0, 0))
    };

    struct CollisionData_t
    {
        GpuProgram_s buildShader;
        GpuProgram_s detectShader;
        GpuProgram_s resetShader;

        using HashGridBuffer_s =
          DerivedBuffer_s<GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY>;
        using HitInfoBuffer_s =
          DerivedBuffer_s<GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY>;
        HashGridBuffer_s hashGridBuffer;
        HitInfoBuffer_s  hitInfoBuffer;
    };
    CollisionData_t m_collision;
};

} // namespace cge

#endif // CGE_WORLDSPAWNER_H
