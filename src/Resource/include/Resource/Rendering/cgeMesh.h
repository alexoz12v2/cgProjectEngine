#pragma once

#include "Core/Containers.h"
#include "Core/Type.h"
#include "Resource/Rendering/Buffer.h"
#include "Resource/Rendering/GpuProgram.h"
#include "Resource/Rendering/cgeTexture.h"

#include <forward_list>
#include <glm/glm.hpp>
#include <map>
#include <memory_resource>
#include <span>
#include <tl/optional.hpp>
#include <vector>

namespace cge
{

using TextureBinderFunc_t = void (*)(struct Mesh_s const *);
inline void noTex(struct Mesh_s const *)
{
    // method used when there are no textures to bind
}

struct Vertex_t
{
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec3 texCoords;
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

struct Mesh_s
{
    static U32_t constexpr uniformCount = 2;

    static constexpr Char8_t const *uniformNames[uniformCount] = {
        "modelView",
        "modelViewProj"
    };

    std::vector<Vertex_t>        vertices;
    std::vector<Array<U32_t, 3>> indices;

    AABB_t box;

    Buffer_s       uniformBuffer;
    VertexBuffer_s vertexBuffer;
    IndexBuffer_s  indexBuffer;
    VertexArray_s  vertexArray;

    // TODO: material information
    std::vector<Sid_t> textures;

    /// @warning to be assigned after the mesh creation, on its first use
    std::vector<Texture_s> uploadedTextures;
    Sid_t                  fragShader;
    GpuProgram_s           shaderProgram; // TODO maybe in handleTable

    /// @warning to be assigned after the mesh creation, on its first use
    // glUniformli(samplerLoc, samplerIndex)
    TextureBinderFunc_t streamTextures = &noTex;

    void streamUniforms(MeshUniform_t const &uniforms) const;

    // Sets up uploadedTextures and streamTextures
    void allocateTexturesToGpu();
    void setupUniforms();
    void allocateGeometryBuffersToGpu();
};

AABB_t computeAABB(const Mesh_s &mesh);

} // namespace cge
