#pragma once

#include "Core/Module.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Player.h"
#include "Render/Renderer.h"
#include "WorldSpawner.h"

namespace cge
{

class TestbedModule : public IModule
{
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
    void onKey(I32_t key, I32_t action);
    void onMouseButton(I32_t key, I32_t action);
    void onMouseMovement(F32_t xPos, F32_t yPos);
    void onFramebufferSize(I32_t width, I32_t height);
    void onTick(float deltaTime) override;

  private:
    void                yawPitchRotate(F32_t yaw, F32_t pitch);
    [[nodiscard]] F32_t aspectRatio() const
    {
        return (F32_t)framebufferSize.x / (F32_t)framebufferSize.y;
    }

    glm::ivec2   framebufferSize{ 800, 600 };
    Player       player;
    WorldSpawner worldSpawner;
};

} // namespace cge
