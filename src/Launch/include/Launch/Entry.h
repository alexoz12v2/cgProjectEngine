#pragma once

#include "Core/Alloc.h"
#include "Core/Module.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"

#include <concepts>
#include <functional>
#include <map>

namespace cge
{

struct ModuleConstructorPair
{
    IModule              *pModule;
    std::function<void()> ctor;
};

using ModuleMap = std::pmr::map<Sid_t, ModuleConstructorPair>;
extern Sid_t g_startupModule;

ModuleMap &getModuleMap();

namespace detail
{
    template<typename T>
        requires(std::derived_from<T, IModule> && !std::same_as<T, IModule>)
    struct Initializer
    {
        Initializer(Char8_t const *moduleStr)
        {
            getModuleMap().at(CGE_SID(moduleStr)).pModule = new T(CGE_SID(moduleStr));
        }
    };

    void addModule(Char8_t const *moduleStr, std::function<void()> const &f);
} // namespace detail
} // namespace cge

#define CGE_DECLARE_STARTUP_MODULE(ModuleNS, ModuleT, moduleStr)                               \
    static ::cge::Char8_t const *name_##ModuleT = (moduleStr);                                 \
    union U##ModuleT;                                                                          \
    void initModuleType##ModuleT(U##ModuleT *ptr);                                             \
    union U##ModuleT                                                                           \
    {                                                                                          \
        U##ModuleT()                                                                           \
        {                                                                                      \
            auto const f = std::function<void()>([this]() { initModuleType##ModuleT(this); }); \
            ::cge::detail::addModule((moduleStr), f);                                          \
            ::cge::g_startupModule = CGE_SID((moduleStr));                                     \
        }                                                                                      \
        ~U##ModuleT() {}                                                                       \
        ::cge::detail::Initializer<::ModuleNS::ModuleT> m;                                     \
    };                                                                                         \
    U##ModuleT g_initModule##ModuleT;                                                          \
    void       initModuleType##ModuleT(U##ModuleT *ptr) { std::construct_at(&ptr->m, (moduleStr)); }

#define CGE_DECLARE_MODULE(ModuleNS, ModuleT, moduleStr)                                       \
    static ::cge::Char8_t const *name_##ModuleT = (moduleStr);                                 \
    union U##ModuleT;                                                                          \
    void initModuleType##ModuleT(U##ModuleT *ptr);                                             \
    union U##ModuleT                                                                           \
    {                                                                                          \
        U##ModuleT()                                                                           \
        {                                                                                      \
            auto const f = std::function<void()>([this]() { initModuleType##ModuleT(this); }); \
            ::cge::detail::addModule((moduleStr), f);                                          \
        }                                                                                      \
        ~U##ModuleT() {}                                                                       \
        ::cge::detail::Initializer<::ModuleNS::ModuleT> m;                                     \
    };                                                                                         \
    U##ModuleT g_initModule##ModuleT;                                                          \
    void       initModuleType##ModuleT(U##ModuleT *ptr) { std::construct_at(&ptr->m, (moduleStr)); }
