#pragma once

#include "Core/Event.h"
#include "Core/Module.h"
#include "Core/Type.h"
#include "Render/Renderer2d.h"
#include "Render/Window.h"

#include "SoundEngine.h"

namespace cge
{

class MenuModule : public IModule
{
  public:
    static U32_t constexpr numButtons = 3;
    static std::array<const char *, numButtons> constexpr names{ "START", "EXTRAS", "EXIT" };
    static inline std::array<ButtonSpec, numButtons> const buttons{
        //
        ButtonSpec{
          .position{ glm::vec2{ 0.05f, 0.7f } },
          .size{ glm::vec2{ 0.2f, 0.1f } },
          .borderColor{ glm::vec3{ 0.5098f, 0.45098f, 0.294118f } },
          .borderWidth{ 0.02f },
          .backgroundColor{ glm::vec3{ 0.2f } },
          .text{ names[0] },
          .textColor{ glm::vec3{ 0.7f } },
        },
        ButtonSpec{
          .position{ glm::vec2{ 0.05f, 0.5f } },
          .size{ glm::vec2{ 0.2f, 0.1f } },
          .borderColor{ glm::vec3{ 0.5098f, 0.45098f, 0.294118f } },
          .borderWidth{ 0.02f },
          .backgroundColor{ glm::vec3{ 0.2f } },
          .text{ names[1] },
          .textColor{ glm::vec3{ 0.7f } },
        },
        ButtonSpec{
          .position{ glm::vec2{ 0.05f, 0.3f } },
          .size{ glm::vec2{ 0.2f, 0.1f } },
          .borderColor{ glm::vec3{ 0.5098f, 0.45098f, 0.294118f } },
          .borderWidth{ 0.02f },
          .backgroundColor{ glm::vec3{ 0.2f } },
          .text{ names[2] },
          .textColor{ glm::vec3{ 0.7f } },
        }
    };

  public:
    MenuModule(Sid_t id) : IModule(id) {}
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
    void buttonPressed(Sid_t buttonSid);

  private:
    // main data
    glm::ivec2 m_framebufferSize{ g_focusedWindow()->getFramebufferSize() };
    glm::vec2  m_mousePosition{ 0.f, 0.f };
    B8_t       m_init{ false };

    // event data
    union
    {
        struct S
        {
            EventDataSidPair framebufferSizeListener;
            EventDataSidPair mouseMovementListener;
            EventDataSidPair mouseButtonListener;
        };
        S s;
        std::array<EventDataSidPair, 3> arr;
        static_assert(std::is_standard_layout_v<S> && sizeof(S) == sizeof(decltype(arr)), "implementation failed");
    } m_listeners{};

    // sound data
    irrklang::ISoundSource *m_bopSource{ nullptr };
    irrklang::ISound       *m_bop{ nullptr };
};

} // namespace cge
