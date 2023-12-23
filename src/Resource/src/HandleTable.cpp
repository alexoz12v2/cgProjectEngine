
#include "HandleTable.h"
#include "Core/Alloc.h"

#include <cassert>

namespace cge
{

HandleTable_s g_handleTable;

void HandleTable_s::insert(Sid_t sid, Resource_s const &ref)
{
    auto const [it, wasInserted] = m_map.try_emplace(sid, 1, ref);

    assert(wasInserted && "ID collision!");
}

B8_t HandleTable_s::remove(Sid_t sid)
{
    auto it = m_map.find(sid);
    if (it != m_map.cend())
    {
        --it->second.first;
        if (it->second.first == 0) { m_map.erase(it); }
    }
}

HandleTable_s::Ref_s HandleTable_s::get(Sid_t sid)
{
    Ref_s ref;
    if (auto it = m_map.find(sid); it != m_map.cend())
    {
        ++it->second.first;
        ref.m_sid    = sid;
        ref.m_ptr    = it;
        ref.m_pTable = this;
    }
    else
    {
        ref.m_sid    = nullSid;
        ref.m_ptr    = m_map.end();
        ref.m_pTable = nullptr;
    }

    return ref;
}

B8_t HandleTable_s::remove(HandleMap_s::iterator it)
{
    if (it != m_map.cend())
    {
        --it->second.first;
        if (it->second.first == 0)
        {
            m_map.erase(it);
            return true;
        }
    }
    return false;
}
} // namespace cge
