#pragma once

#include "Core/Module.h"
#include "Core/Type.h"
#include "Render/Window.h"

namespace cge
{

class MenuModule : public IModule
{
  public:
    MenuModule()                                       = default;
    MenuModule(MenuModule const &other)                = delete;
    MenuModule &operator=(MenuModule const &other)     = delete;
    MenuModule(MenuModule &&other)                     = delete;
    MenuModule &operator=(MenuModule &&other) noexcept = delete;
    ~MenuModule() override                             = default;

  public:
    void onInit(ModuleInitParams params) override;
    void onTick(float deltaTime) override;
    void onFramebufferSize(I32_t width, I32_t height);

  private:
    glm::ivec2 m_framebufferSize{ g_focusedWindow()->getFramebufferSize() };
};

} // namespace cge
