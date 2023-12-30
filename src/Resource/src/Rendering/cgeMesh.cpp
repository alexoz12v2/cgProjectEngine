#include "Rendering/cgeMesh.h"

#include <HandleTable.h>
#include <RenderUtils/GLutils.h>
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

namespace cge
{
AABB_t computeAABB(const Mesh_s &mesh)
{
    if (mesh.vertices.empty())
    {
        // Handle the case when the mesh has no vertices
        // You might want to return an AABB with some default values or handle
        // it differently
        return AABB_t{ glm::vec3(0.0f), glm::vec3(0.0f) };
    }

    // Initialize the min and max coordinates with the first vertex
    glm::vec3 minCoord = mesh.vertices[0].pos;
    glm::vec3 maxCoord = mesh.vertices[0].pos;

    // Iterate through all vertices to find the min and max coordinates
    for (const auto &vertex : mesh.vertices)
    {
        // Update min coordinates
        minCoord = glm::min(minCoord, vertex.pos);

        // Update max coordinates
        maxCoord = glm::max(maxCoord, vertex.pos);
    }

    return AABB_t{ minCoord, maxCoord };
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
    uploadedTextures.reserve(textures.size());
    uploadedTextures.clear();

    U32_t i = 0;
    for (Sid_t texid : textures)
    {
        auto texData = g_handleTable.get(texid).getAsTexture();

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
    streamTextures = [](Mesh_s const *mesh)
    {
        U32_t fragId     = mesh->shaderProgram.id();
        U32_t samplerLoc = glGetUniformLocation(fragId, "sampler");
        glUniform1i(samplerLoc, 0);
    };
}

void Mesh_s::setupUniforms()
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
    layout.push({ .type       = GL_FLOAT,
                  .count      = 3,
                  .targetType = ETargetType::eFloating,
                  .normalized = false });
    layout.push({ .type       = GL_FLOAT,
                  .count      = 3,
                  .targetType = ETargetType::eFloating,
                  .normalized = false });
    layout.push({ .type       = GL_FLOAT,
                  .count      = 3,
                  .targetType = ETargetType::eFloating,
                  .normalized = false });
    vertexArray.addBuffer(vertexBuffer, layout);
}

} // namespace cge
