#include "Module.h"

#include <unordered_map>

namespace cge
{

static std::pmr::unordered_map<Sid_t, bool> moduleInitOnce;

void IModule::tagForDestruction()
{ //
    m_taggedForDestruction = true;
}

IModule::IModule(Sid_t id) : m_id(id)
{
    moduleInitOnce.try_emplace(m_id, false);
}

void IModule::onInit()
{ //
    moduleInitOnce.at(m_id) = true;
}

bool IModule::taggedForDestruction() const
{
    return m_taggedForDestruction;
}

void IModule::switchToModule(Sid_t moduleSid)
{ //
    m_nextModule = moduleSid;
}

bool IModule::initializedOnce() const
{ //
    return moduleInitOnce.at(m_id);
}

Sid_t IModule::moduleSwitched() const
{
    return m_nextModule;
}

void IModule::resetSwitchModule()
{
    m_nextModule = nullSid;
}


std::pmr::unsynchronized_pool_resource *getMemoryPool()
{
    static U32_t constexpr bufferSize = 1U << 10;
    static unsigned char                       poolBuffer[bufferSize];
    static std::pmr::monotonic_buffer_resource poolUpstream{ poolBuffer, bufferSize };

    static std::pmr::unsynchronized_pool_resource g_memoryPool{ std::pmr::pool_options{
                                                                  .max_blocks_per_chunk        = 16,
                                                                  .largest_required_pool_block = 256,
                                                                },
                                                                &poolUpstream };
    return &g_memoryPool;
}

std::pmr::monotonic_buffer_resource *getscratchBuffer()
{
    static U32_t constexpr bufferSize = 4096;
    static unsigned char                       scratchBuffer[bufferSize * bufferSize];
    static std::pmr::monotonic_buffer_resource g_scratchBuffer{ scratchBuffer, bufferSize };
    return &g_scratchBuffer;
}

GlobalStore::GlobalStore() : m_map(getMemoryPool())
{
}

UntypedData128 GlobalStore::consume(Sid_t const &sid)
{
    UntypedData128 res{};
    auto const     it = m_map.find(sid);
    if (it != m_map.cend())
    {
        res = it->second;
        m_map.erase(sid);
    }
    else
    {
        printf("[GlobalStore] didn't find element to consume with sid %zu\n", sid.id);
        assert(false);
    }

    return res;
}

void GlobalStore::put(Sid_t const &sid, UntypedData128 const &data)
{
    auto const &[it, wasInserted] = m_map.try_emplace(sid, data);
    if (!wasInserted)
    {
        printf("[GlobalStore] overwriting value with sid %zu\n", sid.id);
        m_map.erase(sid);
        m_map.try_emplace(sid, data);
    }
}

GlobalStore g_globalStore;

} // namespace cge
