#include "StringUtils.h"

#if defined(CGE_DEBUG)
#include <cstring>
#include <memory>
#include <unordered_map>
#endif


namespace cge
{

#if defined(CGE_DEBUG)

// cannot store char array directly into table as table can get relocated

using StringMap                    = std::unordered_map<U64_t, std::unique_ptr<Char8_t[]>>;
static Char8_t const *const errStr = "Error: String Not Found";

// Avoid static order initialization fiasco (MSVC map implementation)
StringMap &getTable()
{
    static StringMap table;
    return table;
}

Sid_t dbg_internString(Char8_t const *str)
{
    Sid_t strId            = { .id = hashCRC64(str) };
    auto [it, wasInserted] = getTable().try_emplace(strId.id, std::make_unique<Char8_t[]>(1024));
    if (wasInserted)
    {
        strcpy(it->second.get(), str);
// debug print if needed
#if 0
        printf("[StringUtils] Interned new String, table has now size: %zu\n", table.size());
        for (auto const &[num, ptr] : table)
        {
            auto const *const str1 = &ptr[0];
            printf("[StringUtils] \t{ %zu, %s }\n", num, str1);
        }
#endif
        strId.pStr = &it->second[0];
    }
    else if (auto it1 = getTable().find(strId.id); it1 != getTable().cend())
    {
        strId.pStr = &it1->second[0];
    }
    else
    {
        strId.pStr = errStr;
    }
    return strId;
}

Char8_t const *dbg_lookupString(Sid_t sid)
{
    if (sid.pStr != nullptr)
    {
        return sid.pStr;
    }
    else if (auto it = getTable().find(sid.id); it != getTable().cend())
    {
        sid.pStr = &it->second[0];
        return sid.pStr;
    }
    else
    {
        return errStr;
    }
}
#endif
} // namespace cge
