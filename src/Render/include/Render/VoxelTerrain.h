#ifndef CGE_VOXELTERRAIN_H
#define CGE_VOXELTERRAIN_H

#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Resource/Rendering/Buffer.h"
#include "Resource/Rendering/GpuProgram.h"
#include "glad/gl.h"

#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_int3.hpp>

namespace cge
{

struct MarchingCubesSpecs_t
{
    glm::ivec3 size{ 100, 200, 100 };
    float      scale    = 1.f;
    float      isoValue = 0.f;
};

class VoxelMesh_s
{
  public:
    explicit VoxelMesh_s(MarchingCubesSpecs_t const &specs);
    void regenerate(MarchingCubesSpecs_t const &specs, glm::mat4 const &model);
    void draw(
      glm::mat4 const &model,
      glm::mat4 const &view,
      glm::mat4 const &proj);
    ~VoxelMesh_s() { glDeleteBuffers(1, &m_transformBuffer); }

  private:
    struct Triangle_t
    {
        glm::vec4 points[3];
        glm::vec4 normals[3];
    };

    struct DrawArraysIndirectCommand
    {
        U32_t count;
        U32_t instanceCount;
        U32_t first;
        U32_t baseInstance;
    };

    using TriangleBuffer_s =
      DerivedBuffer_s<GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_DRAW>;
    using FloatBuffer_s =
      DerivedBuffer_s<GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY>;
    using TriLUTBuffer_s =
      DerivedBuffer_s<GL_SHADER_STORAGE_BUFFER, GL_STATIC_COPY>;
    using AtomicCounter_s =
      DerivedBuffer_s<GL_ATOMIC_COUNTER_BUFFER, GL_DYNAMIC_DRAW>;
    using IndirectBuffer_s =
      DerivedBuffer_s<GL_DRAW_INDIRECT_BUFFER, GL_STREAM_DRAW>;

    TriangleBuffer_s m_triangleBuffer;
    AtomicCounter_s  m_atomicCounter;
    FloatBuffer_s    m_densityBuffer;
    TriLUTBuffer_s   m_triLUTBuffer;
    IndirectBuffer_s m_indirectBuffer;

    GpuProgram_s m_densityUpdate;
    GpuProgram_s m_voxelCompute;
    GpuProgram_s m_indirectCounter;
    GpuProgram_s m_drawShader;
    Sid_t        m_material = nullSid;

    float m_scale = 0.f;

    GLsync m_fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    // TODO remove
    GLuint m_transformBuffer = 0;

    VertexArray_s m_VAO;
};

} // namespace cge

#endif // CGE_VOXELTERRAIN_H
