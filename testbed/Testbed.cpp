#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Core/Module.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Launch/Entry.h"
#include "Render/Renderer.h"
#include "RenderUtils/GLutils.h"
#include "Resource/Rendering/Buffer.h"
#include "Resource/Rendering/GpuProgram.h"
#include "Resource/Rendering/Mesh.h"
#include "Resource/Rendering/Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <fmt/core.h>
#include <glad/gl.h>
#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>

#define ASSET_PATH \
    "C:\\Users\\oboke\\dev\\info grafica progetto\\cgProjectEngine\\assets\\"

using namespace cge;
static Char8_t const *const vertexSource = R"a(
#version 460 core
out gl_PerVertex
{
    vec4 gl_Position;
};

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec3 aTexCoord;

layout (location = 0) out vec3 vertColor;
layout (location = 1) out vec3 texCoord;

uniform Uniforms {
    vec3 color;
    mat4 modelView;
    mat4 modelViewProjection;
};

void main() 
{
    vertColor = color;
    texCoord = aTexCoord;
    gl_Position = modelViewProjection * vec4(aPos, 1.f);
}
)a";

static Char8_t const *const fragSource = R"a(
#version 460 core
layout (location = 0) in vec3 vertColor;
layout (location = 1) in vec3 texCoord;

uniform sampler2D sampler;

layout (location = 0) out vec4 fragColor;

void main()
{
    vec3 color = texture(sampler, texCoord.xy).xyz * vertColor;
    fragColor = vec4(color, 1.f);
}
)a";

void KeyCallback(EventArg_t eventData, EventArg_t listenerData);
void mouseCallback(EventArg_t eventData, EventArg_t listenerData);
void mouseButtonCallback(EventArg_t eventData, EventArg_t listenerData);
void framebufferSizeCallback(EventArg_t eventData, EventArg_t listenerData);

class TestbedModule : public IModule
{
    static F32_t constexpr baseVelocity     = 10.f;
    static F32_t constexpr mouseSensitivity = 0.1f;
    static Sid_t constexpr vertShader       = "VERTEX"_sid;
    static Sid_t constexpr fragShader       = "FRAG"_sid;

  public:
    void onInit(ModuleInitParams params) override;
    void onKey(I32_t key, I32_t action, F32_t deltaTime);
    void onMouseButton(I32_t key, I32_t action, F32_t deltaTime);
    void onMouseMovement(F32_t xpos, F32_t ypos);
    void onFramebufferSize(I32_t width, I32_t height);
    void onTick(float deltaTime) override;

    void setupUniforms();

    ~TestbedModule() override
    {
        if (m_texData) { stbi_image_free(m_texData); }
    }

  private:
    void  yawPitchRotate(F32_t yaw, F32_t pitch);
    F32_t aspectRatio() const
    {
        return (F32_t)framebufferSize.x / (F32_t)framebufferSize.y;
    }

    glm::ivec2     keyPressed{ 0, 0 }; // WS AD
    glm::ivec2     framebufferSize{ 800, 600 };
    glm::vec2      lastCursorPosition{ -1.0f, -1.0f };
    B8_t           isCursorEnabled = false;
    Camera_t       camera;
    Mesh_s         cubeMesh;
    GpuProgram_s   shaderProgram;
    Buffer_s       uniformBuffer;
    VertexBuffer_s vb;
    IndexBuffer_s  ib;
    VertexArray_s  va;

    Byte_t   *m_texData = nullptr;
    Texture_s texture;

    static Array<F32_t, 9> constexpr vertices = {
        -0.5f, -0.5f, 0.0f, // Vertex 1
        0.5f,  -0.5f, 0.0f, // Vertex 2
        0.0f,  0.5f,  0.0f  // Vertex 3
    };

    // Indices of the triangle (assuming a counter-clockwise winding order)
    static Array<U32_t, 3> constexpr indices = { 0u, 1, 2 };

  public:
    TestbedModule()                                             = default;
    TestbedModule(TestbedModule const &other)                   = delete;
    TestbedModule &operator=(TestbedModule const &other)        = delete;
    bool           operator==(const TestbedModule &other) const = default;
};

CGE_DECLARE_STARTUP_MODULE(TestbedModule, "TestbedModule");


inline void TestbedModule::onInit(ModuleInitParams params)
{
    Sid_t mId = "TestbedModule"_sid;
    CGE_DBG_SID("TestbedModule");
    Char8_t const *str = CGE_DBG_STRLOOKUP(mId);

    fmt::print("Hello World!! {}\n", str);
    {
        auto currentDir = std::filesystem::current_path().string();
        fmt::print("Current Directory: {}", currentDir);
    }

    // register to event key pressed
    EventArg_t listenerData{};
    listenerData.idata.p = (Byte_t *)this;
    g_eventQueue.addListener(evKeyPressed, KeyCallback, listenerData);
    g_eventQueue.addListener(evMouseMoved, mouseCallback, listenerData);
    g_eventQueue.addListener(
      evMouseButtonPressed, mouseButtonCallback, listenerData);
    g_eventQueue.addListener(
      evFramebufferSize, framebufferSizeCallback, listenerData);

    // setup camera
    camera.position = glm::vec3(0.f);
    camera.right    = glm::vec3(1.f, 0.f, 0.f);
    camera.up       = glm::vec3(0.f, 1.f, 0.f);
    camera.forward  = glm::vec3(0.f, 0.f, 1.f);

    // open mesh file
    auto scene = Scene_s::open(ASSET_PATH "cube.obj");
    assert(scene.has_value());
    cubeMesh = scene->getMesh();

    // create shader program
    shaderProgram.bind();
    shaderProgram.addShader({ vertShader, GL_VERTEX_SHADER, vertexSource });
    shaderProgram.addShader({ fragShader, GL_FRAGMENT_SHADER, fragSource });
    shaderProgram.useStages(
      { vertShader, fragShader },
      { GL_VERTEX_SHADER_BIT, GL_FRAGMENT_SHADER_BIT });

    // create texture with default sampler
    I32_t texWidth      = 0;
    I32_t texHeight     = 0;
    I32_t texChannelCnt = 0;
    m_texData           = stbi_load(
      ASSET_PATH "TCom_Gore_2K_albedo.png",
      &texWidth,
      &texHeight,
      &texChannelCnt,
      3 /*RGB*/);
    assert(texWidth != 0);

    glActiveTexture(GL_TEXTURE0 + 0);
    texture.bind(ETexture_t::e2D);
    texture.allocate({ .type           = ETexture_t::e2D,
                       .width          = (U32_t)texWidth,
                       .height         = (U32_t)texHeight,
                       .depth          = 1,
                       .internalFormat = GL_RGBA8,
                       .genMips        = true });
    texture.transferData({ .data   = m_texData,
                           .level  = 0,
                           .xoff   = 0,
                           .yoff   = 0,
                           .zoff   = 0,
                           .width  = (U32_t)texWidth,
                           .height = (U32_t)texHeight,
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
    U32_t fragId     = shaderProgram.id(fragShader);
    U32_t samplerLoc = glGetUniformLocation(fragId, "sampler");
    glUniform1i(samplerLoc, 0);

    // uniform data declaration
    cubeMesh.transform = glm::mat4(1.f);

    static U32_t constexpr uniformCount = 3;
    Char8_t const *names[uniformCount]{ "color",
                                        "modelView",
                                        "modelViewProjection" };
    auto           outUniforms = shaderProgram.getUniformBlock(
      { vertShader, "Uniforms", names, uniformCount });
    uniformBuffer.allocateMutable(
      GL_UNIFORM_BUFFER, outUniforms.blockSize, GL_STATIC_DRAW);
    setupUniforms();

    // create vertex buffer and index buffer
    U32_t vbBytes = (U32_t)cubeMesh.vertices.size() * sizeof(Vertex_t);
    U32_t ibBytes = (U32_t)cubeMesh.indices.size() * sizeof(Array<U32_t, 3>);

    // fill buffers
    va.bind();
    vb.allocateMutable(vbBytes);
    vb.mmap(0, vbBytes, EAccess::eWrite)
      .copyToBuffer(cubeMesh.vertices.data(), vbBytes)
      .unmap();

    ib.allocateMutable(ibBytes);
    ib.mmap(0, ibBytes, EAccess::eWrite)
      .copyToBuffer(cubeMesh.indices.data(), ibBytes)
      .unmap();

    // buffer layout
    vb.bind();
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
    va.addBuffer(vb, layout);
}


// TODO refactor to use view transform and not model

inline void TestbedModule::onKey(I32_t key, I32_t action, F32_t deltaTime)
{
    I32_t dirMult = 0;
    if (action == GLFW_PRESS) { dirMult = 1; }
    if (action == GLFW_RELEASE) { dirMult = -1; }

    switch (key)
    {
    case GLFW_KEY_W:
        keyPressed[0] += dirMult;
        break;
    case GLFW_KEY_A:
        keyPressed[1] += dirMult;
        break;
    case GLFW_KEY_S:
        keyPressed[0] -= dirMult;
        break;
    case GLFW_KEY_D:
        keyPressed[1] -= dirMult;
        break;
    default:
        break;
    }
}

void TestbedModule::onMouseButton(I32_t key, I32_t action, F32_t deltaTime) {}


void TestbedModule::onMouseMovement(F32_t xpos, F32_t ypos)
{
    static F32_t yaw = 0, pitch = 0;
    if (!isCursorEnabled)
    {
        lastCursorPosition = { xpos, ypos };
        isCursorEnabled    = true;
        return;
    }

    F32_t deltaX = lastCursorPosition.x - xpos;
    F32_t deltaY = ypos - lastCursorPosition.y;
    yaw += deltaX * mouseSensitivity;
    pitch += deltaY * mouseSensitivity;

    yawPitchRotate(yaw, pitch);

    lastCursorPosition = { xpos, ypos };
}

void TestbedModule::onFramebufferSize(I32_t width, I32_t height)
{
    framebufferSize.x = width;
    framebufferSize.y = height;
    fmt::print("width: {}, height: {}\n", framebufferSize.x, framebufferSize.y);
}

void TestbedModule::onTick(float deltaTime)
{
    F32_t velocity = baseVelocity * deltaTime;

    // Calculate the movement direction based on camera's forward vector
    auto direction = (F32_t)keyPressed[0] * camera.forward
                     + (F32_t)keyPressed[1] * camera.right;
    if (direction != glm::vec3(0.f)) { direction = glm::normalize(direction); }

    // Update the camera position
    camera.position += velocity * direction;

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glClearColor(0.f, 0.f, 0.f, 1.f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    shaderProgram.bind();
    va.bind();

    U32_t fragId     = shaderProgram.id(fragShader);
    U32_t samplerLoc = glGetUniformLocation(fragId, "sampler");
    glUniform1i(samplerLoc, 0);
    setupUniforms();

    glDrawElements(
      GL_TRIANGLES,
      (U32_t)cubeMesh.indices.size() * 3,
      GL_UNSIGNED_INT,
      nullptr);
}

void TestbedModule::setupUniforms()
{

    // uniform data declaration
    static U32_t constexpr uniformCount = 3;
    Char8_t const *names[uniformCount]{ "color",
                                        "modelView",
                                        "modelViewProjection" };
    Byte_t         uniformData[1024];
    auto           outUniforms = shaderProgram.getUniformBlock(
      { vertShader, "Uniforms", names, uniformCount });

    // copy data to local array
    float color[] = { 1.f, 0.f, 0.f };
    std::memcpy(
      (Byte_t *)(uniformData) + outUniforms.uniformOffset[0],
      color,
      outUniforms.uniformSize[0] * typeSize(outUniforms.uniformType[0]));

    glm::mat4 modelView = camera.viewTransform() * cubeMesh.transform;
    std::memcpy(
      (Byte_t *)(uniformData) + outUniforms.uniformOffset[1],
      glm::value_ptr(modelView),
      outUniforms.uniformSize[1] * typeSize(outUniforms.uniformType[1]));

    glm::mat4 modelViewProjection =
      glm::perspective(45.f, aspectRatio(), 0.1f, 100.f);
    modelViewProjection = modelViewProjection * modelView;
    std::memcpy(
      (Byte_t *)(uniformData) + outUniforms.uniformOffset[2],
      glm::value_ptr(modelViewProjection),
      outUniforms.uniformSize[2] * typeSize(outUniforms.uniformType[2]));

    // bind uniform buffer and copy data to GPU
    uniformBuffer
      .mmap(GL_UNIFORM_BUFFER, 0, outUniforms.blockSize, EAccess::eWrite)
      .copyToBuffer(uniformData, outUniforms.blockSize)
      .unmap();

    U32_t ubo = uniformBuffer.id();
    glBindBufferBase(GL_UNIFORM_BUFFER, outUniforms.blockIdx, ubo);
}

void TestbedModule::yawPitchRotate(F32_t yaw, F32_t pitch)
{
    yaw   = glm::radians(yaw);
    pitch = glm::radians(glm::clamp(pitch, -89.f, 89.f));

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 direction;
    direction.x    = cos(yaw) * cos(pitch);
    direction.y    = sin(pitch);
    direction.z    = sin(yaw) * cos(pitch);
    camera.forward = glm::normalize(direction);
    camera.up      = glm::vec3(0.f, 1.f, 0.f);
    camera.right   = glm::normalize(glm::cross(camera.forward, camera.up));
    camera.up      = glm::normalize(glm::cross(camera.forward, camera.right));
}

void KeyCallback(EventArg_t eventData, EventArg_t listenerData)
{
    auto self = (TestbedModule *)listenerData.idata.p;
    self->onKey(
      eventData.idata.i32[0], eventData.idata.i32[1], eventData.fdata.f32[0]);
}

void mouseButtonCallback(EventArg_t eventData, EventArg_t listenerData)
{
    auto self = (TestbedModule *)listenerData.idata.p;
    self->onMouseButton(
      eventData.idata.i32[0], eventData.idata.i32[1], eventData.fdata.f32[0]);
};

void mouseCallback(EventArg_t eventData, EventArg_t listenerData)
{
    auto self = (TestbedModule *)listenerData.idata.p;
    self->onMouseMovement(eventData.fdata.f32[0], eventData.fdata.f32[1]);
}

void framebufferSizeCallback(EventArg_t eventData, EventArg_t listenerData)
{
    auto self = (TestbedModule *)listenerData.idata.p;
    self->onFramebufferSize(eventData.idata.i32[0], eventData.idata.i32[1]);
}
