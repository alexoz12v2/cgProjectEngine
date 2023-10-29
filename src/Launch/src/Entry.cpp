// MIT License. Copyright (c) 2023

#include "Entry.h"

#include "Core/Module.h"
#include "Core/Type.h"

// change in ISceneModule
IModule *g_startupModule = nullptr;

template<std::derived_from<IModule> T> void declareStartupModule() {}

I32_t main(I32_t argc, Char8_t **argv)
{
    ModuleInitParams const params{};
    g_startupModule->onInit(params);
    return 0;
}
