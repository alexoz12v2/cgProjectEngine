#pragma once

#include "Launch/Entry.h"

namespace cge
{
class MainMenuModule : public IModule
{
  public:
    void onInit(ModuleInitParams params) override;
    void onTick(float deltaTime) override;
};
}