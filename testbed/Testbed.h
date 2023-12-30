#pragma once

#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Core/Module.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Launch/Entry.h"
#include "Render/Renderer.h"
#include "Resource/Rendering/GpuProgram.h"
#include "Resource/Rendering/cgeTexture.h"

#include <glm/glm.hpp>
#include <stb/stb_image.h>

namespace cge
{

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
    static Sid_t constexpr cubeMeshSid      = "Cube"_sid;
    static Sid_t constexpr planeMeshSid     = "Plane"_sid;

  public:
    void onInit(ModuleInitParams params) override;
    void onKey(I32_t key, I32_t action, cge::F32_t deltaTime);
    void onMouseButton(I32_t key, I32_t action, cge::F32_t deltaTime);
    void onMouseMovement(F32_t xpos, F32_t ypos);
    void onFramebufferSize(I32_t width, I32_t height);
    void onTick(float deltaTime) override;

  private:
    void  yawPitchRotate(F32_t yaw, F32_t pitch);
    F32_t aspectRatio() const
    {
        return (F32_t)framebufferSize.x / (F32_t)framebufferSize.y;
    }

    glm::ivec2 keyPressed{ 0, 0 }; // WS AD
    glm::ivec2 framebufferSize{ 800, 600 };
    glm::vec2  lastCursorPosition{ -1.0f, -1.0f };
    B8_t       isCursorEnabled = false;
    Camera_t   camera;
    AABB_t     box;

  public:
    TestbedModule()                                      = default;
    TestbedModule(TestbedModule const &other)            = delete;
    TestbedModule &operator=(TestbedModule const &other) = delete;
};

} // namespace cge
