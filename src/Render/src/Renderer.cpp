#include "Renderer.h"

#include <Resource/HandleTable.h>
#include <glad/gl.h>
#include <glm/ext/matrix_transform.hpp>

namespace cge
{

Renderer_s g_renderer;

glm::mat4 Camera_t::viewTransform() const
{
    return glm::lookAt(position, position + forward, up);
}
void Renderer_s::renderScene(
  Scene_s const   &scene,
  glm::mat4 const &view,
  glm::mat4 const &proj) const
{
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glClearColor(0.f, 0.f, 0.f, 1.f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    for (auto const &[sid, sceneNode] : scene.m_bnodes)
    {
        auto          meshRef = g_handleTable.get(sid);
        Mesh_s const &mesh    = meshRef.getAsMesh();

        glm::mat4 const     modelView = view * sceneNode.absoluteTransform;
        MeshUniform_t const uniforms{ .modelView     = modelView,
                                      .modelViewProj = proj * modelView };

        mesh.shaderProgram.bind();
        mesh.vertexArray.bind();
        mesh.streamTextures(&mesh);
        mesh.streamUniforms(uniforms);

        glDrawElements(
          GL_TRIANGLES,
          (U32_t)mesh.indices.size() * 3,
          GL_UNSIGNED_INT,
          nullptr);
    }
}

void Renderer_s::renderCube() const
{
    static U32_t constexpr verticesCount               = 8;
    static glm::vec3 constexpr vertices[verticesCount] = {
        // -------------------
        // Front face
        { -1.f, -1.f, 1.f },
        { 1.f, -1.f, 1.f },
        { 1.f, 1.f, 1.f },
        { -1.f, 1.f, 1.f },
        // Back face
        { -1.f, -1.f, -1.f },
        { 1.f, -1.f, -1.f },
        { 1.f, 1.f, -1.f },
        { -1.f, 1.f, -1.f }
    };
    static U32_t constexpr verticesNumComponents = glm::vec3::length();
    static U32_t constexpr verticesBytes =
      verticesNumComponents * verticesCount * sizeof(glm::vec3::value_type);

    static U32_t constexpr triangleCount                 = 12;
    static glm::uvec3 constexpr triangles[triangleCount] = {
        // --------------------
        // Front Face
        { 0, 1, 3 },
        { 1, 2, 3 },
        // Back Face
        { 5, 4, 7 },
        { 6, 5, 7 },
        // Left Face
        { 0, 3, 7 },
        { 4, 0, 7 },
        // Right Face
        { 1, 5, 2 },
        { 5, 6, 2 },
        // Top Face
        { 3, 2, 6 },
        { 7, 3, 6 },
        // Bottom Face
        { 4, 5, 1 },
        { 0, 4, 1 }
    };
    static U32_t constexpr indicesNumComponents = glm::uvec3::length();
    static U32_t constexpr indicesBytes =
      indicesNumComponents * triangleCount * sizeof(glm::uvec3::value_type);

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verticesBytes, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
      GL_ELEMENT_ARRAY_BUFFER, indicesBytes, triangles, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Render the cube
    glBindVertexArray(VAO);
    glDrawElements(
      GL_TRIANGLES, indicesNumComponents * triangleCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}
void Renderer_s::clear() const {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void Renderer_s::viewport(U32_t width, U32_t height) const {
    glViewport(0, 0, width, height);
}
} // namespace cge
