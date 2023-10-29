#pragma once
// MIT License. Copyright (c) 2023

#include <concepts>

#include "Core/Module.h"
#include "Core/Type.h"

// change in ISceneModule (maybe remove)
extern IModule *g_startupModule;

I32_t main(I32_t argc, Char8_t **argv);

namespace detail {
template<std::derived_from<IModule> T> struct Initializer
{
    // todo Refactor
    Initializer() { g_startupModule = new T(); }
    ~Initializer() { delete g_startupModule; }
};

// TODO add debug
#define CGE_DECLARE_STARTUP_MODULE(ModuleT) \
    static detail::Initializer<ModuleT> const g_init##ModuleT;

};// namespace detail
