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
  public:
    explicit TestbedModule(Sid_t id);
    TestbedModule(TestbedModule const &other)                = delete;
    TestbedModule &operator=(TestbedModule const &other)     = delete;
    TestbedModule(TestbedModule &&other)                     = delete;
    TestbedModule &operator=(TestbedModule &&other) noexcept = delete;
    ~TestbedModule() override;

  public:
    void onInit() override;
    void onTick(U64_t deltaTime) override;
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
    enum class EGameState {
        eDefault = 0,
        eDead
    };

    glm::ivec2 m_framebufferSize{ g_focusedWindow()->getFramebufferSize() };
    Player     m_player;
    U32_t      m_letterSize = 0;
    EGameState m_gameState = EGameState::eDefault;
    F32_t      m_fov;
    F32_t      m_targetFov;
    U32_t      m_numCoins = 0;

    // terrain data
    ScrollingTerrain m_scrollingTerrain;

    // event data
    union
    {
        struct S
        {
            EventDataSidPair keyListener;
            EventDataSidPair mouseMovementListener;
            EventDataSidPair mouseButtonListener;
            EventDataSidPair framebufferSizeListener;

            EventDataSidPair gameOverListener;
            EventDataSidPair shootListener;
            EventDataSidPair magnetAcquiredListener;
        };
        S                               s;
        std::array<EventDataSidPair, 7> arr;
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
