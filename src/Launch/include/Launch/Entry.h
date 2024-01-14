#pragma once

#include <concepts>

#include "Core/Alloc.h"
#include "Core/Module.h"
#include "Core/Type.h"
#include <functional>

namespace cge
{

extern IModule               *g_startupModule;
extern Pool_t                 g_pool;
extern std::function<void()> *g_constructStartupModule;

I32_t main(I32_t argc, Char8_t **argv);

extern void enableCursor();
extern void disableCursor();

namespace detail
{
    EErr_t initMemory();
    template<typename T>
        requires(std::derived_from<T, IModule> && !std::same_as<T, IModule>)
    struct Initializer
    {
        // TODO: engine initialization doesn't start here anymore. Refactor in
        // functions
        /**
         * @warning note that initialization of the engine starts here with the
         * allocation of the startup module. No construction, only allocation as
         * all the engines subsystems are still to load here
         */
        Initializer(Char8_t const *moduleStr)
        {
            /**
             * @warning it is required that before the macro @ref
             * CGE_DECLARE_STARTUP_MODULE the module is fully specified with
             * refl macros
             */
            initMemory();
            /// @warning g_pool is BUGGED, it overwrites memory of the allocated
            /// object. debug it if you have time
            // g_pool.allocate<T>(
            //   reinterpret_cast<T **>(&g_startupModule),
            //   CGE_DBG_SID(moduleStr),
            //   false);
            g_startupModule = new T();
            // std::construct_at((T *)g_startupModule, T{});
        }
    };

// TODO: allocate properly
// drawback: you need header memory
#define CGE_DECLARE_STARTUP_MODULE(ModuleNS, ModuleT, moduleStr)             \
    static ::cge::Char8_t const *name_##ModuleT = (moduleStr);               \
    union U##ModuleT;                                                        \
    void initModuleType(U##ModuleT *ptr);                                    \
    union U##ModuleT                                                         \
    {                                                                        \
        U##ModuleT()                                                         \
        {                                                                    \
            ::cge::g_constructStartupModule =                                \
              new std::function<void()>([this]() { initModuleType(this); }); \
        }                                                                    \
        ~U##ModuleT() {}                                                     \
        ::cge::detail::Initializer<::ModuleNS::ModuleT> m;                   \
    };                                                                       \
    U##ModuleT g_initModule;                                                 \
    void       initModuleType(U##ModuleT *ptr)                               \
    {                                                                        \
        std::construct_at(&ptr->m, (moduleStr));                             \
    }
} // namespace detail
} // namespace cge
