#pragma once

#include "Core/Event.h"
#include "Core/Module.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Render/Renderer.h"
#include "Render/VoxelTerrain.h"
#include "Resource/Rendering/cgeTexture.h"
#include "WorldSpawner.h"

#include <stb/stb_image.h>

namespace cge
{

class TestbedModule : public IModule
{
    static F32_t constexpr baseVelocity     = 10.f;
    static F32_t constexpr mouseSensitivity = 0.1f;
    static Sid_t constexpr vertShader       = "VERTEX"_sid;
    static Sid_t constexpr fragShader       = "FRAG"_sid;
    static Sid_t constexpr cubeMeshSid      = "Cube"_sid;
    static Sid_t constexpr planeMeshSid     = "Plane"_sid;

  public:
    TestbedModule();
    TestbedModule(TestbedModule const &other)                = delete;
    TestbedModule &operator=(TestbedModule const &other)     = delete;
    TestbedModule(TestbedModule &&other)                     = delete;
    TestbedModule &operator=(TestbedModule &&other) noexcept = delete;
    ~TestbedModule() override                                = default;

    void onInit(ModuleInitParams params) override;
    void onKey(I32_t key, I32_t action, cge::F32_t deltaTime);
    void onMouseButton(I32_t key, I32_t action, cge::F32_t deltaTime);
    void onMouseMovement(F32_t xpos, F32_t ypos);
    void onFramebufferSize(I32_t width, I32_t height);
    void onTick(float deltaTime) override;

  private:
    void                yawPitchRotate(F32_t yaw, F32_t pitch);
    [[nodiscard]] F32_t aspectRatio() const
    {
        return (F32_t)framebufferSize.x / (F32_t)framebufferSize.y;
    }

    glm::ivec2   keyPressed{ 0, 0 }; // WS AD
    glm::ivec2   framebufferSize{ 800, 600 };
    glm::vec2    lastCursorPosition{ -1.0f, -1.0f };
    B8_t         isCursorDisabled = false;
    Camera_t     camera{};
    AABB_t       box{};
    WorldSpawner worldSpawner;
};

} // namespace cge
