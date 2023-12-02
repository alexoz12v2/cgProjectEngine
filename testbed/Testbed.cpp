#include "Core/Module.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Launch/Entry.h"

#include <fmt/core.h>

class TestbedModule : public IModule
{
  public:
    void onInit(ModuleInitParams params) override
    {
        Sid_t mId = "TestbedModule"_sid;
        CGE_DBG_SID("TestbedModule");
        Char8_t const* str = CGE_DBG_STRLOOKUP(mId);

        fmt::print("Hello World!! {}", str);
    }

    ~TestbedModule() override = default;
};

CGE_DECLARE_STARTUP_MODULE(TestbedModule, "TestbedModule");
