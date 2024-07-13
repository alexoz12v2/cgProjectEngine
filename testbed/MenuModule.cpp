#include "MenuModule.h"

#include "Core/Event.h"
#include "Core/Events.h"
#include "Launch/Entry.h"
#include "Render/Renderer.h"
#include "Render/Renderer2d.h"

CGE_DECLARE_MODULE(cge, MenuModule, "MenuModule");

namespace cge
{

void MenuModule::onInit(ModuleInitParams params)
{
    EventArg_t listenerData{};
    listenerData.idata.p =
      reinterpret_cast<decltype(listenerData.idata.p)>(this);
    g_eventQueue.addListener(
      evFramebufferSize, framebufferSizeCallback<MenuModule>, listenerData);
}

void MenuModule::onTick(float deltaTime)
{
    ButtonSpec const spec{
        .position{ glm::vec2{ 0.05f, 0.7f } },
        .size{ glm::vec2{ 0.2f, 0.1f } },
        .borderColor{ glm::vec3{ 0.5098f, 0.45098f, 0.294118f } },
        .borderWidth{ 0.02f },
        .backgroundColor{ glm::vec3{ 0.2f } },
        .text{ "BUTTON" },
        .textColor{ glm::vec3{ 0.7f } },
    };
    g_renderer2D.renderButton(spec);
}


void MenuModule::onFramebufferSize(I32_t width, I32_t height)
{
    m_framebufferSize.x = width;
    m_framebufferSize.y = height;
}

} // namespace cge
