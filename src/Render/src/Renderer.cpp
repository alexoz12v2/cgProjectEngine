#include "Renderer.h"

#include "Core/Event.h"
#include "Core/Events.h"

#include <Resource/HandleTable.h>
#include <glad/gl.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace cge
{

Renderer_s g_renderer;

BackgroundRenderer &getBackgroundRenderer()
{
    static BackgroundRenderer g_backgroundRenderer;
    return g_backgroundRenderer;
}

glm::mat4 Camera_t::viewTransform() const
{
    return glm::lookAt(position, position + forward, up);
}

void Camera_t::setForward(glm::vec3 newForward)
{
    // Normalize the new forward vector
    forward = glm::normalize(newForward);

    // Calculate a temporary right vector based on the cross product of
    // world up (usually Y-axis) and the new forward
    right = glm::normalize(glm::cross(glm::vec3(0.f, 0.f, 1.f), forward));

    // Calculate the final up vector using the cross product of the new
    // forward and the temporary right vector
    up = glm::normalize(glm::cross(forward, right));
}

void Renderer_s::init()
{ //
    EventArg_t listenerData{};
    listenerData.idata.p =
      reinterpret_cast<decltype(listenerData.idata.p)>(this);
    g_eventQueue.addListener(
      evFramebufferSize, framebufferSizeCallback<Renderer_s>, listenerData);
}

void Renderer_s::renderScene(
  Scene_s const   &scene,
  glm::mat4 const &view,
  glm::mat4 const &proj) const
{
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    for (auto const &[sid, sceneNode] : scene.m_bnodes)
    {
        auto          meshRef = g_handleTable.get(sid);
        Mesh_s const &mesh    = meshRef.asMesh();

        glm::mat4 const     modelView = view * sceneNode.getAbsoluteTransform();
        MeshUniform_t const uniforms{ .modelView     = modelView,
                                      .modelViewProj = proj * modelView };

        mesh.shaderProgram.bind();
        mesh.vertexArray.bind();
        mesh.bindTextures(&mesh);
        mesh.streamUniforms(uniforms);

        glDrawElements(
          GL_TRIANGLES,
          (U32_t)mesh.indices.size() * 3,
          GL_UNSIGNED_INT,
          nullptr);
    }
    glUseProgram(0);
}

// TODO parameters: transform, drawMode, normalOrientation
void Renderer_s::renderCube() const
{
    static U32_t constexpr verticesCount     = 8;
    static glm::vec3 vertices[verticesCount] = {
        // -------------------------------
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
    static U32_t verticesNumComponents = glm::vec3::length();
    static U32_t verticesBytes =
      verticesNumComponents * verticesCount * sizeof(glm::vec3::value_type);

    static U32_t constexpr triangleCount       = 12;
    static glm::uvec3 triangles[triangleCount] = {
        // ----------------------------
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
    static U32_t indicesNumComponents = glm::uvec3::length();
    static U32_t indicesBytes =
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

void Renderer_s::clear() const
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer_s::viewport(U32_t width, U32_t height) const
{
    glViewport(0, 0, width, height);
}

void Renderer_s::onFramebufferSize(I32_t width, I32_t height)
{
    viewport(width, height);
}

// BUG: you can use this function only once. Otherwise, background is black
void BackgroundRenderer::init(unsigned char *image, I32_t width, I32_t height)
{
    if (m_init) //
        return;

    if (!image) { assert(false); }

    Texture_s background;
    background.bind(ETexture_t::e2D);
    background.allocate({ .width          = (U32_t)width,
                          .height         = (U32_t)height,
                          .internalFormat = GL_RGB8,
                          .genMips        = true });
    background.transferData({ .data   = image,
                              .width  = (U32_t)width,
                              .height = (U32_t)height,
                              .format = GL_RGB,
                              .type   = GL_UNSIGNED_BYTE });
    background.defaultSamplerParams({ .minFilter = GL_LINEAR_MIPMAP_LINEAR,
                                      .magFilter = GL_LINEAR,
                                      .wrap      = GL_REPEAT });
    auto optVert = g_shaderLibrary.open("../assets/Background.vert");
    auto optFrag = g_shaderLibrary.open("../assets/Background.frag");
    if (!optVert.has_value() || !optFrag.has_value()) { assert(false); }
    const Shader_s *ppShaders[2] = { *optVert, *optFrag };
    m_backgrProgram.build("background", ppShaders, 2);

    // cubemap which will host the background
    m_cubeBackground.bind(ETexture_t::eCube);
    static U32_t constexpr CUBE_FRAMEBUFFER_SIZE = 1024;
    for (unsigned int i = 0; i < 6; ++i)
    {
        // note that we store each face with 16 bit floating point values
        glTexImage2D(
          GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
          0,
          GL_RGB8,
          CUBE_FRAMEBUFFER_SIZE,
          CUBE_FRAMEBUFFER_SIZE,
          0,
          GL_RGB,
          GL_FLOAT,
          nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned int captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(
      GL_RENDERBUFFER,
      GL_DEPTH_COMPONENT24,
      CUBE_FRAMEBUFFER_SIZE,
      CUBE_FRAMEBUFFER_SIZE);
    glFramebufferRenderbuffer(
      GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    static glm::mat4 const captureProjection =
      glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    static glm::mat4 const captureViews[] = //
      { glm::lookAt(
          glm::vec3(0.0f, 0.0f, 0.0f),
          glm::vec3(1.0f, 0.0f, 0.0f),
          glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(
          glm::vec3(0.0f, 0.0f, 0.0f),
          glm::vec3(-1.0f, 0.0f, 0.0f),
          glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(
          glm::vec3(0.0f, 0.0f, 0.0f),
          glm::vec3(0.0f, 1.0f, 0.0f),
          glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(
          glm::vec3(0.0f, 0.0f, 0.0f),
          glm::vec3(0.0f, -1.0f, 0.0f),
          glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(
          glm::vec3(0.0f, 0.0f, 0.0f),
          glm::vec3(0.0f, 0.0f, 1.0f),
          glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(
          glm::vec3(0.0f, 0.0f, 0.0f),
          glm::vec3(0.0f, 0.0f, -1.0f),
          glm::vec3(0.0f, -1.0f, 0.0f)) };

    // convert HDR equirectangular environment map to cubemap equivalent
    GpuProgram_s equirectangularToCubemapShader;
    auto         opt1 = g_shaderLibrary.open("../assets/EquiToCube.vert");
    auto         opt2 = g_shaderLibrary.open("../assets/EquiToCube.frag");
    if (!opt1.has_value() || !opt2.has_value()) { assert(false); }
    Shader_s const *ppCubeShaders[2] = { *opt1, *opt2 };
    equirectangularToCubemapShader.build("equi to cube", ppCubeShaders, 2);
    equirectangularToCubemapShader.bind();
    glUniformMatrix4fv(
      glGetUniformLocation(equirectangularToCubemapShader.id(), "projection"),
      1,
      GL_FALSE,
      &captureProjection[0][0]);
    glActiveTexture(GL_TEXTURE0);
    background.bind(ETexture_t::e2D);

    // don't forget to configure the viewport to the capture dimensions.
    glViewport(0, 0, CUBE_FRAMEBUFFER_SIZE, CUBE_FRAMEBUFFER_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("Framebuffer is not complete!");
    }
    for (unsigned int i = 0; i < 6; ++i)
    {
        glUniformMatrix4fv(
          glGetUniformLocation(equirectangularToCubemapShader.id(), "view"),
          1,
          GL_FALSE,
          &captureViews[i][0][0]);
        glFramebufferTexture2D(
          GL_FRAMEBUFFER,
          GL_COLOR_ATTACHMENT0,
          GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
          m_cubeBackground.id(),
          0);
        glClearColor(0, 0, 0, 0);
        glClear(GL_DEPTH_BUFFER_BIT);
        glClear(GL_COLOR_BUFFER_BIT);

        g_renderer.renderCube(); // renders a 1x1 cube
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);

    m_init = true;
}

void BackgroundRenderer::renderBackground(
  Camera_t const &camera,
  F32_t           aspectRatio,
  F32_t           clipDistance,
  F32_t           renderDistance) const
{ //
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    m_cubeBackground.bind(ETexture_t::eCube);
    m_backgrProgram.bind();
    glm::mat4 rotationMatrix =
      glm::lookAt(glm::vec3(0.F), camera.forward, camera.up);

    glUniformMatrix4fv(
      glGetUniformLocation(m_backgrProgram.id(), "view"),
      1,
      GL_FALSE,
      &rotationMatrix[0][0]);
    F32_t ratio = aspectRatio;
    ratio       = ratio >= 1.f ? ratio : 1.f;
    auto proj   = glm::perspective(45.F, ratio, clipDistance, renderDistance);
    glUniformMatrix4fv(
      glGetUniformLocation(m_backgrProgram.id(), "projection"),
      1,
      GL_FALSE,
      &proj[0][0]);
    glUniform3fv(
      glGetUniformLocation(m_backgrProgram.id(), "cameraPos"),
      1,
      &camera.position[0]);

    g_renderer.renderCube();
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}


} // namespace cge
