#include "MenuModule.h"

#include "Core/Event.h"
#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Launch/Entry.h"
#include "Render/Renderer.h"
#include "Render/Renderer2d.h"

CGE_DECLARE_MODULE(cge, MenuModule, "MenuModule");

namespace cge
{
MenuModule::~MenuModule()
{
    if (m_init)
    {
        for (auto const &pair : m_listeners.arr)
        { //
            g_eventQueue.removeListener(pair);
        }
    }
}

void MenuModule::onInit(ModuleInitParams params)
{
    EventArg_t listenerData{};
    listenerData.idata.p =
      reinterpret_cast<decltype(listenerData.idata.p)>(this);
    m_listeners.framebufferSizeListener = g_eventQueue.addListener(
      evFramebufferSize, framebufferSizeCallback<MenuModule>, listenerData);
    m_listeners.mouseMovementListener = g_eventQueue.addListener(
      evMouseMoved, mouseMovementCallback<MenuModule>, listenerData);
    m_listeners.mouseButtonListener = g_eventQueue.addListener(
      evMouseButtonPressed, mouseButtonCallback<MenuModule>, listenerData);

    m_init = true;
}

void MenuModule::onTick(F32_t deltaTime)
{
    for (const auto &button : buttons)
    { //
        g_renderer2D.renderButton(button);
    }
}


void MenuModule::onFramebufferSize(I32_t width, I32_t height)
{
    m_framebufferSize.x = width;
    m_framebufferSize.y = height;
}

void MenuModule::onMouseMovement(F32_t x, F32_t y)
{
    m_mousePosition = glm::vec2(x, m_framebufferSize.y - y)
                      / static_cast<glm::vec2>(m_framebufferSize);
}

void MenuModule::onMouseButton(I32_t key, I32_t action)
{
    if (action == action::PRESS)
    {
        printf(
          "[MenuModule] position: \n\t%f\n\t%f\n",
          m_mousePosition.x,
          m_mousePosition.y);
        for (const auto &button : buttons)
        {
            printf(
              "[MenuModule] button: \n\t%f - %f\n\t%f - %f\n",
              button.position.x,
              button.position.x + button.size.x,
              button.position.y,
              button.position.y + button.size.y);
            if (
              m_mousePosition.x >= button.position.x
              && m_mousePosition.y >= button.position.y
              && m_mousePosition.x < button.position.x + button.size.x
              && m_mousePosition.y < button.position.y + button.size.y)
            { //
                printf("[MenuModule] pressed button \"%s\"\n", button.text);
                buttonPressed(CGE_SID(button.text));
            }
        }
    }
}

void MenuModule::buttonPressed(Sid_t buttonSid)
{ //
    switch (buttonSid.id)
    {
    case "BUTTON"_sid.id:
        switchToModule(CGE_SID("TestbedModule"));
        break;
    }
}

} // namespace cge
