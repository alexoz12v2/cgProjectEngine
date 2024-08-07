#pragma once

#include "Core/Containers.h"
#include "Core/Module.h"
#include "Core/Type.h"
#include "Resource/Rendering/Buffer.h"
#include "Resource/Rendering/GpuProgram.h"
#include "Resource/Rendering/cgeTexture.h"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

#include <vector>

// TODO: Tear this class to pieces and figure out something else. See FScene
namespace cge
{

using TextureBinderFunc_t = void (*)(struct Mesh_s const *);

struct Vertex_t
{
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec3 texCoords;
    glm::vec4 color;
    F32_t     shininess;
};

// necessary to use offsetof macro
static_assert(std::is_standard_layout_v<Vertex_t>);

struct MeshUniform_t
{
    glm::mat4 modelView;
    glm::mat4 modelViewProj;
};
// needed to use memcpy
static_assert(std::is_standard_layout_v<MeshUniform_t>);

union Textures_t
{ //
    constexpr Textures_t()
      : albedo(nullSid), normal(nullSid), shininess(nullSid)
    {
    }
    constexpr ~Textures_t() {}
    struct
    {
        Sid_t albedo;
        Sid_t normal;
        Sid_t shininess;
    };
    std::array<Sid_t, 3> arr;
};

struct Mesh_s
{
    static U32_t constexpr uniformCount = 2;

    static constexpr Char8_t const *uniformNames[uniformCount] = {
        "modelView",
        "modelViewProj"
    };

    std::pmr::vector<Vertex_t>        vertices{ getMemoryPool() };
    std::pmr::vector<Array<U32_t, 3>> indices{ getMemoryPool() };

    AABB box{ glm::vec3(0.f), glm::vec3(0.f) };

    Buffer_s       uniformBuffer;
    VertexBuffer_s vertexBuffer;
    IndexBuffer_s  indexBuffer;
    VertexArray_s  vertexArray;

    // TODO: material information
    Textures_t textures;
    U32_t      numTextures;

    /// @warning to be assigned after the mesh creation, on its first use
    std::vector<Texture_s> uploadedTextures;
    Sid_t                  fragShader;
    GpuProgram_s           shaderProgram; // TODO maybe in handleTable

    /// @warning to be assigned after the mesh creation, on its first use
    // glUniformli(samplerLoc, samplerIndex)
    TextureBinderFunc_t bindTextures;

    B8_t hasDiffuse  = false;
    B8_t hasNormal   = false;
    B8_t hasSpecular = false;

    void streamUniforms(MeshUniform_t const &uniforms) const;

    // Sets up uploadedTextures and bindTextures
    void allocateTexturesToGpu();
    void setupUniforms() const;
    void allocateGeometryBuffersToGpu();
};

AABB computeAABB(const Mesh_s &mesh);

} // namespace cge
