#include "WorldSpawner.h"

#include "Core/Event.h"
#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Core/Type.h"
#include "Render/Renderer.h"
#include "Resource/Rendering/Buffer.h"
#include "Resource/Rendering/GpuProgram.h"
#include "Resource/Rendering/ShaderLibrary.h"
#include "Resource/Rendering/cgeTexture.h"

#include <glad/gl.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <stb/stb_image.h>

namespace cge
{
void WorldSpawner::init()
{
    EventArg_t listenerData{};
    listenerData.idata.p = (Byte_t *)this;
    g_eventQueue.addListener(
      evFramebufferSize, framebufferSizeCallback<WorldSpawner>, listenerData);

    // background
    generateBackgroundCubemap();

    // terrain collision initialization
    createCollisionBuffersAndShaders();
}

void WorldSpawner::generateBackgroundCubemap()
{
    stbi_set_flip_vertically_on_load(true);
    const char    *imagePath = "../assets/background.png";
    int            width, height, channels;
    unsigned char *image =
      stbi_load(imagePath, &width, &height, &channels, STBI_rgb);
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

    stbi_image_free(image);

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

    glm::mat4 captureProjection =
      glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = { glm::lookAt(
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
}
void WorldSpawner::renderTerrain(Camera_t const &camera)
{
    for (U32_t i = 0; i != chunksCount; i++)
    {
        m_terrain[i].regenerate({ .scale = 2.F }, m_terrainTransform[i]);
    }
    m_terrain[0].draw(
      glm::mat4(1.F),
      camera.viewTransform(),
      glm::perspective(FOV, aspectRatio(), CLIPDISTANCE, RENDERDISTANCE));
    m_terrain[1].draw(
      glm::mat4(1.F),
      camera.viewTransform(),
      glm::perspective(FOV, aspectRatio(), CLIPDISTANCE, RENDERDISTANCE),
      { 1.f, 0, 0 });
}

void WorldSpawner::renderBackground(Camera_t const &camera) const
{
    glFrontFace(GL_CW);
    glEnable(GL_CULL_FACE);
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
    F32_t ratio = aspectRatio();
    ratio       = ratio >= 1.f ? ratio : 1.f;
    auto proj   = glm::perspective(45.F, ratio, CLIPDISTANCE, RENDERDISTANCE);
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
    glFrontFace(GL_CCW);
}

void WorldSpawner::onFramebufferSize(U32_t width, U32_t height)
{
    m_framebufferSize.x = width;
    m_framebufferSize.y = height;
}
F32_t WorldSpawner::aspectRatio() const
{
    return (F32_t)m_framebufferSize.x / (F32_t)m_framebufferSize.y;
}

void WorldSpawner::transformTerrain(const glm::mat4 &transform)
{
    // for (U32_t i = 0; i != chunksCount; i++)
    //{
    //     m_terrainTransform[i] *= transform;
    // }
}

HitInfo_t WorldSpawner::detectTerrainCollisions(const glm::mat4 &transform)
{
    // m_collision.buildShader.bind();
    // glUniform1ui(glGetUniformLocation(m_collision.buildShader.id(),
    // "nextFree"), 0u);
    return HitInfo_t{};
}

struct ListNode
{
    U32_t triangleIndex;
    U32_t nextNode; // if NULL_NODE then nothing
};

void WorldSpawner::createCollisionBuffersAndShaders()
{
    // build all shader programs
    g_shaderLibrary.open("../assets/BuildTerrainHashGrid.comp")
      .map_or_else(
        [this](Shader_s const *shader)
        { m_collision.buildShader.build("BuildTerrainHashGrid", &shader, 1); },
        []() { printf("could not open BuildTerrainHashGrid shader"); });
    g_shaderLibrary.open("../assets/TerrainCollision.comp")
      .map_or_else(
        [this](Shader_s const *shader)
        { m_collision.detectShader.build("TerrainCollision", &shader, 1); },
        []() { printf("could not open TerrainCollision shader"); });
    g_shaderLibrary.open("../assets/ResetTerrainHashGrid.comp")
      .map_or_else(
        [this](Shader_s const *shader)
        { m_collision.resetShader.build("ResetTerrainHashGrid", &shader, 1); },
        []() { printf("could not open ResetTerrainHashGrid shader"); });

    // allocate all buffers
    glUniform1ui(
      glGetUniformLocation(m_collision.buildShader.id(), "nextFree"), 0u);

    static U32_t constexpr axisSize = 512u * sizeof(U32_t);
    // 32 MB for a total of 2^22 list nodes
    static U32_t constexpr listSize = (1u << 22u) * sizeof(ListNode);
    m_collision.hashGridBuffer.allocateImmutable(
      axisSize + listSize, EAccess::eNone);

    static HitInfo_t const initial{ .present  = false,
                                    .position = glm::vec3(0.F),
                                    .normal   = glm::vec3(0.F) };
    m_collision.hitInfoBuffer.allocateMutable(sizeof(HitInfo_t));
    m_collision.hitInfoBuffer.transferDataImm(0, sizeof(initial), &initial);

    // bind buffers to their correct indexed bindings
    bindCollisionbuffers();

    // run resetTerrainhashGrid.comp to initialize it
    m_collision.resetShader.bind();
    glDispatchCompute(1, 1, 1);
    m_collision.resetShader.unbind();
}

void WorldSpawner::bindCollisionbuffers() const
{
    glBindBufferBase(
      decltype(m_collision.hashGridBuffer)::targetType,
      9,
      m_collision.hashGridBuffer.id());
    glBindBufferBase(
      decltype(m_collision.hitInfoBuffer)::targetType,
      10,
      m_collision.hitInfoBuffer.id());
}

} // namespace cge