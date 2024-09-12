#include "MenuModule.h"

#include "Core/Event.h"
#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Launch/Entry.h"
#include "Render/Renderer2d.h"
#include "Resource/HandleTable.h"

#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <stb/stb_image.h>

CGE_DECLARE_MODULE(cge, MenuModule, "MenuModule");

static nlohmann::json json;

namespace cge
{

// constants
inline U32_t constexpr numButtons = 3;
inline std::array<const char *, numButtons> constexpr names{ "START", "EXTRAS", "EXIT" };
inline std::array<ButtonSpec, numButtons> const buttons{
    // ----------------------------------------------------------------
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

struct STBIDeleter
{
    void operator()(Byte_t *data) const
    {
        stbi_image_free(data);
    }
};

MenuModule::~MenuModule()
{
    if (m_init)
    {
        for (auto const &pair : m_listeners.arr)
        {
            g_eventQueue.removeListener(pair);
        }

        if (m_bop)
        {
            m_bop->stop();
            m_bop->drop();
        }

        g_soundEngine()->removeSoundSource(m_bopSource);
    }
}

void MenuModule::onInit()
{
    EventArg_t listenerData{};
    listenerData.idata.p = reinterpret_cast<decltype(listenerData.idata.p)>(this);
    m_listeners.s.framebufferSizeListener =
      g_eventQueue.addListener(evFramebufferSize, framebufferSizeCallback<MenuModule>, listenerData);
    m_listeners.s.mouseMovementListener =
      g_eventQueue.addListener(evMouseMoved, mouseMovementCallback<MenuModule>, listenerData);
    m_listeners.s.mouseButtonListener =
      g_eventQueue.addListener(evMouseButtonPressed, mouseButtonCallback<MenuModule>, listenerData);

    m_bopSource = g_soundEngine()->addSoundSourceFromFile("../assets/bop.mp3");

    std::ifstream file{ "../assets/saves.json" };
    if (file)
    {
        json = nlohmann::json::parse(file);
    }
    else
    {
        std::vector<U64_t> empty;
        json = nlohmann::json::object({ { "bestScore", empty } });
    }

    if (!initializedOnce())
    {
        stbi_set_flip_vertically_on_load(true);
        I32_t texWidth      = 0;
        I32_t texHeight     = 0;
        I32_t texChannelCnt = 0;
        auto *tex           = reinterpret_cast<Byte_t *>(
          stbi_load("../assets/menu.png", &texWidth, &texHeight, &texChannelCnt, STBI_rgb_alpha));
        assert(texChannelCnt == 4);
        std::shared_ptr<Byte_t> data(tex, STBIDeleter{});
        g_handleTable.insertTexture(
          CGE_SID("MENU"),
          { .data{ data },
            .width  = static_cast<U32_t>(texWidth),
            .height = static_cast<U32_t>(texHeight),
            .depth  = 1,
            .format = GL_RGBA,
            .type   = GL_UNSIGNED_BYTE });
    }

    IModule::onInit();
    m_init = true;
}

void MenuModule::onTick(U64_t deltaTime)
{
    g_renderer2D.renderTexture({ .position{ 0.f, 0.f },
                                 .size{ 1.f, 1.f },
                                 .texture{ CGE_SID("MENU") },
                                 .renderMode = ETextureRenderMode::ConstantSizeNoStretching });
    for (const auto &button : buttons)
    {
        g_renderer2D.renderButton(button);
    }

    g_renderer2D.renderRectangle({ .position{ 0.7f, 0.5f }, .size{ 0.2f, 0.05f }, .color{ 0.5f } });
}

void MenuModule::onFramebufferSize(I32_t width, I32_t height)
{
    m_framebufferSize.x = width;
    m_framebufferSize.y = height;
}

void MenuModule::onMouseMovement(F32_t x, F32_t y)
{
    m_mousePosition = glm::vec2(x, m_framebufferSize.y - y) / static_cast<glm::vec2>(m_framebufferSize);
}

void MenuModule::onMouseButton(I32_t key, I32_t action)
{
    if (action == action::PRESS)
    {
        printf("[MenuModule] position: \n\t%f\n\t%f\n", m_mousePosition.x, m_mousePosition.y);
        for (const auto &button : buttons)
        {
            printf(
              "[MenuModule] button: \n\t%f - %f\n\t%f - %f\n",
              button.position.x,
              button.position.x + button.size.x,
              button.position.y,
              button.position.y + button.size.y);
            if (
              m_mousePosition.x >= button.position.x && m_mousePosition.y >= button.position.y
              && m_mousePosition.x < button.position.x + button.size.x
              && m_mousePosition.y < button.position.y + button.size.y)
            {
                printf("[MenuModule] pressed button \"%s\"\n", button.text);
                m_bop = g_soundEngine()->play2D(m_bopSource);
                buttonPressed(CGE_SID(button.text));
                break;
            }
        }
    }
}

void MenuModule::buttonPressed(Sid_t buttonSid)
{
    switch (buttonSid.id)
    {
    case CGE_CONSTEXPR_SID(names[0]).id:
        switchToModule(CGE_SID("TestbedModule"));
        break;
    case CGE_CONSTEXPR_SID(names[2]).id:
        tagForDestruction();
        break;
    }
}

} // namespace cge
