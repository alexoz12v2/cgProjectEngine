#pragma once

#include "Core/Event.h"
#include "Core/Module.h"
#include "Core/Type.h"
#include "Render/Renderer2d.h"
#include "Render/Window.h"

#include "ConstantsAndStructs.h"
#include "SoundEngine.h"

namespace cge
{

class MenuModule : public IModule
{
  public:
    MenuModule(Sid_t id) : IModule(id)
    {
    }
    MenuModule(MenuModule const &other)                = delete;
    MenuModule &operator=(MenuModule const &other)     = delete;
    MenuModule(MenuModule &&other)                     = delete;
    MenuModule &operator=(MenuModule &&other) noexcept = delete;
    ~MenuModule() override;

  public:
    void onInit() override;
    void onTick(U64_t deltaTime) override;
    void onFramebufferSize(I32_t width, I32_t height);
    void onMouseMovement(F32_t x, F32_t y);
    void onMouseButton(I32_t key, I32_t action);

  private:
    void buttonPressed(Sid_t buttonSid, B8_t isDifficulty, I32_t key);
    void deserializeScoresFromFile();
    void serializeScoresToFile();

  private:
    // main data
    std::pmr::vector<U64_t> m_scores{10, getMemoryPool()};
    glm::ivec2 m_framebufferSize{ g_focusedWindow()->getFramebufferSize() };
    glm::vec2  m_mousePosition{ 0.f, 0.f };
    B8_t       m_init{ false };
    EDifficulty m_difficulty{EDifficulty::eNormal};

    // selected menu
    enum class EMenuScreen
    {
        eMain = 0,
        eExtras
    };
    EMenuScreen m_menuScreen{EMenuScreen::eMain};

    // event data
    union
    {
        struct S
        {
            EventDataSidPair framebufferSizeListener;
            EventDataSidPair mouseMovementListener;
            EventDataSidPair mouseButtonListener;
        };
        S                               s;
        std::array<EventDataSidPair, 3> arr;
        static_assert(std::is_standard_layout_v<S> && sizeof(S) == sizeof(decltype(arr)), "implementation failed");
    } m_listeners{};

    // sound data
    irrklang::ISoundSource *m_bopSource{ nullptr };
    irrklang::ISound       *m_bop{ nullptr };
    F32_t                   aspectRatio() const;
};

} // namespace cge
