#pragma once

#include "Core/Event.h"
#include "Core/Module.h"
#include "Core/Type.h"
#include "Render/Renderer2d.h"
#include "Render/Window.h"

namespace cge
{

class MenuModule : public IModule
{
  public:
    static U32_t constexpr numButtons = 2;
    static std::array<const char *, numButtons> constexpr names{
        "START", ///////
        "EXIT"
    };
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
          .position{ glm::vec2{ 0.05f, 0.3f } },
          .size{ glm::vec2{ 0.2f, 0.1f } },
          .borderColor{ glm::vec3{ 0.5098f, 0.45098f, 0.294118f } },
          .borderWidth{ 0.02f },
          .backgroundColor{ glm::vec3{ 0.2f } },
          .text{ names[1] },
          .textColor{ glm::vec3{ 0.7f } },
        }
    };

  public:
    MenuModule()                                       = default;
    MenuModule(MenuModule const &other)                = delete;
    MenuModule &operator=(MenuModule const &other)     = delete;
    MenuModule(MenuModule &&other)                     = delete;
    MenuModule &operator=(MenuModule &&other) noexcept = delete;
    ~MenuModule() override;

  public:
    void onInit(ModuleInitParams params) override;
    void onTick(F32_t deltaTime) override;
    void onFramebufferSize(I32_t width, I32_t height);
    void onMouseMovement(F32_t x, F32_t y);
    void onMouseButton(I32_t key, I32_t action);

  private:
    void buttonPressed(Sid_t buttonSid);

  private:
    glm::ivec2 m_framebufferSize{ g_focusedWindow()->getFramebufferSize() };
    glm::vec2  m_mousePosition{ 0.f, 0.f };
    union
    {
        struct
        {
            std::pair<Event_t, Sid_t> framebufferSizeListener;
            std::pair<Event_t, Sid_t> mouseMovementListener;
            std::pair<Event_t, Sid_t> mouseButtonListener;
        };
        std::array<std::pair<Event_t, Sid_t>, 3> arr;
    } m_listeners{};
    B8_t m_init{ false };
};

} // namespace cge
