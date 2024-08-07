#include "Rendering/cgeMesh.h"
#include "Rendering/Buffer.h"

#include <HandleTable.h>
#include <RenderUtils/GLutils.h>
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

namespace cge
{
AABB computeAABB(const Mesh_s &mesh)
{
    AABB aabb{ glm::vec3(0.0f), glm::vec3(0.0f) };
    aabb.min = glm::vec3(std::numeric_limits<float>::max());
    aabb.max = glm::vec3(std::numeric_limits<float>::min());

    // Loop through each indexed vertex
    for (size_t i = 0; i < mesh.indices.size(); ++i)
    {
        for (U32_t j : mesh.indices[i])
        {
            const auto &vertex = mesh.vertices[j];

            // Update AABB based on vertex position
            aabb.min = glm::min(aabb.min, vertex.pos);
            aabb.max = glm::max(aabb.max, vertex.pos);
        }
    }

    return aabb;
}

void Mesh_s::streamUniforms(MeshUniform_t const &uniforms) const
{
    static Byte_t uniformData[1024];
    auto          outUniforms =
      shaderProgram.getUniformBlock({ .blockName    = "MeshUniforms",
                                      .uniformNames = uniformNames,
                                      .uniformCount = uniformCount });
    std::memcpy(
      (Byte_t *)(uniformData) + outUniforms.uniformOffset[0],
      glm::value_ptr(uniforms.modelView),
      outUniforms.uniformSize[0] * typeSize(outUniforms.uniformType[0]));
    std::memcpy(
      (Byte_t *)(uniformData) + outUniforms.uniformOffset[1],
      glm::value_ptr(uniforms.modelViewProj),
      outUniforms.uniformSize[1] * typeSize(outUniforms.uniformType[1]));

    // bind uniform buffer and copy data to GPU
    uniformBuffer
      .mmap(GL_UNIFORM_BUFFER, 0, outUniforms.blockSize, EAccess::eWrite)
      .copyToBuffer(uniformData, outUniforms.blockSize)
      .unmap();

    U32_t ubo = uniformBuffer.id();
    glBindBufferBase(GL_UNIFORM_BUFFER, outUniforms.blockIdx, ubo);
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

        // TODO error handling with some default texture
    }

    // assign default texture binder function (TODO better)
    if (!textures.arr.empty())
    {
        bindTextures = [](Mesh_s const *mesh)
        {
            static char const *namesBools[3]{ "hasAlbedoTexture",
                                              "hasNormalSampler",
                                              "hasShininessSampler" };
            static char const *namesSamplers[3]{ "albedoSampler",
                                                 "normalSampler",
                                                 "shininessSampler" };
            U32_t              fragId = mesh->shaderProgram.id();

            for (U32_t i = 0; i != 3; ++i)
            {
                if (
                  (i == 0 && mesh->hasDiffuse) || (i == 1 && mesh->hasNormal)
                  || (i == 2 && mesh->hasSpecular))
                {
                    glActiveTexture(GL_TEXTURE0 + i);
                    glBindTexture(
                      GL_TEXTURE_2D, mesh->uploadedTextures[i].id());

                    // U32_t samplerLoc =
                    // glGetUniformLocation(fragId, namesSamplers[i]);
                    // glUniform1i(samplerLoc, 0);
                    glUniform1i(
                      glGetUniformLocation(fragId, namesBools[i]), true);
                }
            }
        };
    }
    else
    { // method used when there are no textures to bind
        bindTextures = [](Mesh_s const *mesh)
        {
            static char const *namesBools[3]{ "hasAlbedoTexture",
                                              "hasNormalSampler",
                                              "hasShininessSampler" };
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
    auto uniforms = shaderProgram.getUniformBlock(
      { "MeshUniforms", uniformNames, uniformCount });
    uniformBuffer.allocateMutable(
      GL_UNIFORM_BUFFER, uniforms.blockSize, GL_STATIC_DRAW);
}
void Mesh_s::allocateGeometryBuffersToGpu()
{
    // create vertex buffer and index buffer
    U32_t vbBytes = (U32_t)vertices.size() * sizeof(Vertex_t);
    U32_t ibBytes = (U32_t)indices.size() * sizeof(Array<U32_t, 3>);

    // fill buffers
    vertexArray.bind();
    vertexBuffer.allocateMutable(vbBytes);
    vertexBuffer.mmap(0, vbBytes, EAccess::eWrite)
      .copyToBuffer(vertices.data(), vbBytes)
      .unmap();

    indexBuffer.allocateMutable(ibBytes);
    indexBuffer.mmap(0, ibBytes, EAccess::eWrite)
      .copyToBuffer(indices.data(), ibBytes)
      .unmap();

    // buffer layout
    vertexBuffer.bind();
    BufferLayout_s layout;
    // -- aPos
    layout.push({ .type       = GL_FLOAT,
                  .count      = 3,
                  .targetType = ETargetType::eFloating,
                  .normalized = false });
    // -- aNorm
    layout.push({ .type       = GL_FLOAT,
                  .count      = 3,
                  .targetType = ETargetType::eFloating,
                  .normalized = false });
    // -- aTexCoord
    layout.push({ .type       = GL_FLOAT,
                  .count      = 3,
                  .targetType = ETargetType::eFloating,
                  .normalized = false });
    // -- aColor
    layout.push({ .type       = GL_FLOAT,
                  .count      = 4,
                  .targetType = ETargetType::eFloating,
                  .normalized = false });
    // -- aShininess
    layout.push({ .type       = GL_FLOAT,
                  .count      = 1,
                  .targetType = ETargetType::eFloating,
                  .normalized = false });
    vertexArray.addBuffer(vertexBuffer, layout);
}

} // namespace cge
