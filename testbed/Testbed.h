#pragma once

#include "Core/Event.h"
#include "Core/Module.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Core/Utility.h"
#include "Player.h"
#include "Render/Window.h"

#include <irrKlang/irrKlang.h>

namespace cge
{

class TestbedModule : public IModule
{
  private:
    static Sid_t constexpr vertShader     = "VERTEX"_sid;
    static Sid_t constexpr fragShader     = "FRAG"_sid;
    static Sid_t constexpr cubeMeshSid    = "Cube"_sid;
    static Sid_t constexpr planeMeshSid   = "Plane"_sid;
    static U32_t constexpr coinBonusScore = 100;

  public:
    explicit TestbedModule(Sid_t id) : IModule(id) {}
    TestbedModule(TestbedModule const &other)                = delete;
    TestbedModule &operator=(TestbedModule const &other)     = delete;
    TestbedModule(TestbedModule &&other)                     = delete;
    TestbedModule &operator=(TestbedModule &&other) noexcept = delete;
    ~TestbedModule() override;

  public:
    void onInit(ModuleInitParams params) override;
    void onTick(float deltaTime) override;
    void onKey(I32_t key, I32_t action);
    void onMouseButton(I32_t key, I32_t action);
    void onMouseMovement(F32_t xPos, F32_t yPos);
    void onFramebufferSize(I32_t width, I32_t height);
    void onGameOver(U64_t score);
    void onShoot(Ray const &ray);
    void onMagnetAcquired();

  private:
    [[nodiscard]] F32_t aspectRatio() const;

    // Assuming clickPos is in normalized device coordinates [-1, 1], with origin at center
    B8_t isAnyCoinClicked(glm::vec2 const &clickPos);

  private:
    glm::ivec2 m_framebufferSize{ g_focusedWindow()->getFramebufferSize() };
    Player     m_player;

    // terrain data
    ScrollingTerrain m_scrollingTerrain;

    // event data
    union
    {
        struct S
        {
            std::pair<Event_t, Sid_t> keyListener;
            std::pair<Event_t, Sid_t> mouseMovementListener;
            std::pair<Event_t, Sid_t> mouseButtonListener;
            std::pair<Event_t, Sid_t> framebufferSizeListener;

            std::pair<Event_t, Sid_t> gameOverListener;
            std::pair<Event_t, Sid_t> shootListener;
            std::pair<Event_t, Sid_t> magnetAcquiredListener;
        };
        S s;
        std::array<std::pair<Event_t, Sid_t>, 7> arr;
        static_assert(std::is_standard_layout_v<S> && sizeof(S) == sizeof(decltype(arr)), "implementation failed");
    } m_listeners{};
    B8_t m_init{ false };

    // mouse
    glm::vec2 m_screenMousePos{ 0.f, 0.f };

    // sound
    irrklang::ISoundSource *m_coinPickedSource{ nullptr };
    irrklang::ISoundSource *m_woodBreakSource{ nullptr };
};

} // namespace cge
