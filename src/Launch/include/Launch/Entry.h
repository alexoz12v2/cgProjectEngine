#pragma once

#include <concepts>

#include "Core/Alloc.h"
#include "Core/Module.h"
#include "Core/Type.h"

// change in ISceneModule (maybe remove)
extern IModule *g_startupModule;
extern Pool_t   g_pool;

I32_t main(I32_t argc, Char8_t **argv);

namespace detail
{
EErr_t initMemory();
template<typename T>
    requires(std::derived_from<T, IModule> && !std::same_as<T, IModule>)
struct Initializer
{
    /**
     * @warning note that initialization of the engine starts here with the allocation of the
     * startup module. No construction, only allocation as all the engines subsystems are still to
     * load here
     */
    Initializer(Char8_t const *moduleStr)
    {
        /**
         * @warning it is required that before the macro @ref CGE_DECLARE_STARTUP_MODULE the
         * module is fully specified with refl macros
         */
        initMemory();
        g_pool.allocate<T>(
          reinterpret_cast<T **>(&g_startupModule), CGE_DBG_SID(moduleStr), false, true, T{});
    }
};

// TODO add debug
#define CGE_DECLARE_STARTUP_MODULE(ModuleT, moduleStr) \
    static detail::Initializer<ModuleT> const g_init##ModuleT{(moduleStr)};
} // namespace detail
