#pragma once

#include "Core/Module.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Player.h"
#include "Render/Renderer.h"
#include "Render/Window.h"
#include "WorldSpawner.h"

namespace cge
{

class TestbedModule : public IModule
{
  private:
    static Sid_t constexpr vertShader   = "VERTEX"_sid;
    static Sid_t constexpr fragShader   = "FRAG"_sid;
    static Sid_t constexpr cubeMeshSid  = "Cube"_sid;
    static Sid_t constexpr planeMeshSid = "Plane"_sid;

  public:
    TestbedModule()                                          = default;
    TestbedModule(TestbedModule const &other)                = delete;
    TestbedModule &operator=(TestbedModule const &other)     = delete;
    TestbedModule(TestbedModule &&other)                     = delete;
    TestbedModule &operator=(TestbedModule &&other) noexcept = delete;
    ~TestbedModule() override                                = default;

  public:
    void onInit(ModuleInitParams params) override;
    void onTick(float deltaTime) override;
    void onKey(I32_t key, I32_t action);
    void onMouseButton(I32_t key, I32_t action);
    void onMouseMovement(F32_t xPos, F32_t yPos);
    void onFramebufferSize(I32_t width, I32_t height);

  private:
    [[nodiscard]] F32_t aspectRatio() const;

  private:
    glm::ivec2 m_framebufferSize{ g_focusedWindow()->getFramebufferSize() };
    Player                  m_player;
    WorldSpawner            m_worldSpawner;
    ScrollingTerrain        m_scrollingTerrain;
    std::pmr::vector<Sid_t> m_pieces;
    std::pmr::vector<Sid_t> m_obstacles;
};

} // namespace cge
