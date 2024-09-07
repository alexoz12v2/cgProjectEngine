#include "Rendering/cgeMesh.h"

#include "Core/Type.h"
#include "Core/Utility.h"
#include "HandleTable.h"
#include "RenderUtils/GLutils.h"
#include "Rendering/Buffer.h"

#include <glad/gl.h>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <limits>
#include <numeric>

namespace cge
{
AABB computeAABB(Mesh_s const &mesh)
{
    // Initialize min and max to extreme values
    AABB aabb{ glm::vec3{ std::numeric_limits<F32_t>::max() }, glm::vec3{ std::numeric_limits<F32_t>::lowest() } };

    // Loop through each indexed vertex
    for (const auto &indexList : mesh.indices)
    {
        for (U32_t index : indexList)
        {
            const glm::vec3 &vertexPos = mesh.vertices[index].pos;

            // Update AABB based on vertex position
            aabb.mm.min = glm::min(aabb.mm.min, vertexPos);
            aabb.mm.max = glm::max(aabb.mm.max, vertexPos);
        }
    }

    return aabb;
}
void Mesh_s::streamUniforms(MeshUniform_t const &uniforms) const
{
    static Byte_t uniformData[1024];
    auto          outUniforms = shaderProgram.getUniformBlock(
      { .blockName = "MeshUniforms", .uniformNames = uniformNames, .uniformCount = uniformCount });
    std::memcpy(
      reinterpret_cast<Byte_t *>(uniformData) + outUniforms.uniformOffset[0],
      glm::value_ptr(uniforms.modelView),
      outUniforms.uniformSize[0] * typeSize(outUniforms.uniformType[0]));
    std::memcpy(
      reinterpret_cast<Byte_t *>(uniformData) + outUniforms.uniformOffset[1],
      glm::value_ptr(uniforms.modelViewProj),
      outUniforms.uniformSize[1] * typeSize(outUniforms.uniformType[1]));
    std::memcpy(
      reinterpret_cast<Byte_t *>(uniformData) + outUniforms.uniformOffset[2],
      glm::value_ptr(uniforms.model),
      outUniforms.uniformSize[2] * typeSize(outUniforms.uniformType[2]));

    // bind uniform buffer and copy data to GPU
    uniformBuffer.mmap(GL_UNIFORM_BUFFER, 0, outUniforms.blockSize, EAccess::eWrite)
      .copyToBuffer(uniformData, outUniforms.blockSize)
      .unmap();

    U32_t ubo = uniformBuffer.id();
    glBindBufferBase(GL_UNIFORM_BUFFER, outUniforms.blockIdx, ubo);
}

static B8_t isThereAnyTexture(Textures_t const &textures)
{
    return std::accumulate(
      std::begin(textures.arr),
      std::end(textures.arr),
      false,
      [](B8_t &&accumulator, Sid_t const &current) -> B8_t { return accumulator || current != nullSid; });
}

void Mesh_s::allocateTexturesToGpu()
{
    uploadedTextures.reserve(64); // reserve is no op for counts < 64
    uploadedTextures.reserve(textures.arr.size());
    uploadedTextures.clear();

    U32_t i = 0;
    for (Sid_t texid : textures.arr)
    {
        if (texid == nullSid)
        { //
            continue;
        }
        auto &texData = g_handleTable.get(texid).asTexture();

        // upload texture to gpu
        glActiveTexture(GL_TEXTURE0 + i);

        uploadedTextures.emplace_back();
        Texture_s &texture = uploadedTextures.back();

        texture.bind(ETexture_t::e2D);
        texture.allocate({ .type           = ETexture_t::e2D,
                           .width          = texData.width,
                           .height         = texData.height,
                           .depth          = 1,
                           .internalFormat = GL_RGBA8,
                           .genMips        = true });
        texture.transferData({ .data   = texData.data.get(),
                               .level  = 0,
                               .xoff   = 0,
                               .yoff   = 0,
                               .zoff   = 0,
                               .width  = texData.width,
                               .height = texData.height,
                               .depth  = 1,
                               .layer  = 0,
                               .format = GL_RGB,
                               .type   = GL_UNSIGNED_BYTE });
        texture.defaultSamplerParams({
          .minFilter   = GL_LINEAR_MIPMAP_LINEAR,
          .magFilter   = GL_LINEAR,
          .minLod      = -1000.f, // default
          .maxLod      = 1000.f,
          .wrap        = GL_REPEAT,
          .borderColor = {},
        });
    }

    static char const *namesBools[3]{ "hasAlbedoTexture", "hasNormalSampler", "hasShininessSampler" };
    if (numTextures > 0)
    {
        bindTextures = [](Mesh_s const *mesh)
        {
            static char const *namesSamplers[3]{ "albedoSampler", "normalSampler", "specularSampler" };
            B8_t texturePresentArr[3]{ mesh->hasDiffuse, mesh->hasNormal, mesh->hasSpecular };
            U32_t              fragId = mesh->shaderProgram.id();

            for (U32_t i = 0; i != 3; ++i)
            {
                glActiveTexture(GL_TEXTURE0 + i);
                if (texturePresentArr[i]) {
                    glBindTexture(GL_TEXTURE_2D, mesh->uploadedTextures[i].id());

                    // Bind points are declared in the shader, so this isn't necessary
                    // U32_t samplerLoc = glGetUniformLocation(fragId, namesSamplers[i]);
                    // glUniform1i(samplerLoc, i);
                    glUniform1i(glGetUniformLocation(fragId, namesBools[i]), GL_TRUE);
                }
                else
                {
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glUniform1i(glGetUniformLocation(fragId, namesBools[i]), GL_FALSE);
                }
            }
        };
    }
    else
    { // method used when there are no textures to bind
        bindTextures = [](Mesh_s const *mesh)
        {
            U32_t              fragId = mesh->shaderProgram.id();
            for (auto const &n : namesBools)
            {
                glUniform1i(glGetUniformLocation(fragId, n), false);
            }
        };
    }
}

void Mesh_s::setupUniforms() const
{
    auto uniforms = shaderProgram.getUniformBlock({ "MeshUniforms", uniformNames, uniformCount });
    uniformBuffer.allocateMutable(GL_UNIFORM_BUFFER, uniforms.blockSize, GL_STATIC_DRAW);
}
void Mesh_s::allocateGeometryBuffersToGpu()
{
    // create vertex buffer and index buffer
    U32_t vbBytes = (U32_t)vertices.size() * sizeof(Vertex_t);
    U32_t ibBytes = (U32_t)indices.size() * sizeof(Array<U32_t, 3>);

    // fill buffers
    vertexArray.bind();
    vertexBuffer.allocateMutable(vbBytes);
    vertexBuffer.mmap(0, vbBytes, EAccess::eWrite).copyToBuffer(vertices.data(), vbBytes).unmap();

    indexBuffer.allocateMutable(ibBytes);
    indexBuffer.mmap(0, ibBytes, EAccess::eWrite).copyToBuffer(indices.data(), ibBytes).unmap();

    // buffer layout
    vertexBuffer.bind();
    BufferLayout_s layout;
    // -- aPos
    layout.push({ .type = GL_FLOAT, .count = 3, .targetType = ETargetType::eFloating, .normalized = false });
    // -- aNorm
    layout.push({ .type = GL_FLOAT, .count = 3, .targetType = ETargetType::eFloating, .normalized = false });
    // -- aTexCoord
    layout.push({ .type = GL_FLOAT, .count = 3, .targetType = ETargetType::eFloating, .normalized = false });
    // -- aColor
    layout.push({ .type = GL_FLOAT, .count = 4, .targetType = ETargetType::eFloating, .normalized = false });
    // -- aShininess
    layout.push({ .type = GL_FLOAT, .count = 1, .targetType = ETargetType::eFloating, .normalized = false });
    vertexArray.addBuffer(vertexBuffer, layout);
}

} // namespace cge
