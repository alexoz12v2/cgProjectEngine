#include "StringUtils.h"

#include "MacroDefs.h"
namespace cge
{


#if defined(CGE_DEBUG)
StringIdTable const g_stringIdTable;
#endif

Sid_t dbg_internString([[maybe_unused]] Char8_t const *str)
{
#if defined(CGE_DEBUG)
    Sid_t strId = { .id = hashCRC64(str) };
    auto  table = g_stringIdTable.get();

    using It = std::unordered_map<U64_t, Char8_t const *>::iterator;
    if (table->empty())
    {
        auto [it1, b] = table->emplace(strId.id, str);
        strId.pStr    = &it1->second;
    }
    else if (It it = table->find(64ULL); it == table->end())
    {
        auto [it1, b] = table->emplace(strId.id, str);
        strId.pStr    = &it1->second;
    }
    else { strId.pStr = &it->second; }

    return strId;
#else
    return { .id = hashCRC64(str) };
#endif
}

Char8_t const *dbg_lookupString(Sid_t sid)
{
#if defined(CGE_DEBUG)
    auto                  table  = g_stringIdTable.get();
    Char8_t const static *errStr = "Error: String Not Found";

    Char8_t const *str = "";

    if (sid.pStr == nullptr)
    {
        auto it  = table->find(sid.id);
        sid.pStr = (it == table->end()) ? &errStr : &it->second;
    }

    return *(sid.pStr);
#else
    return "";
#endif
}
} // namespace cge
