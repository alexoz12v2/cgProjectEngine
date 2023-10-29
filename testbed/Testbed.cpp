
#include <fmt/core.h>

#include "Core/Module.h"
#include "Launch/Entry.h"

class TestbedModule : public IModule
{
  public:
    void onInit(ModuleInitParams params) override { fmt::print("Hello World!!"); }

    ~TestbedModule() override = default;
};

CGE_DECLARE_STARTUP_MODULE(TestbedModule);
