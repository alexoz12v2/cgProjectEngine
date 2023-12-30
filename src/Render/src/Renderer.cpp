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

} // namespace cge
