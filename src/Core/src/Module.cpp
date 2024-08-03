#include "Module.h"

namespace cge
{

void IModule::tagForDestruction()
{ //
    m_taggedForDestruction = true;
}

bool IModule::taggedForDestruction() const { return m_taggedForDestruction; }

void IModule::switchToModule(Sid_t moduleSid)
{ //
    m_nextModule = moduleSid;
}

Sid_t IModule::moduleSwitched() const { return m_nextModule; }

void IModule::resetSwitchModule() { m_nextModule = nullSid; }


std::pmr::unsynchronized_pool_resource *getMemoryPool()
{
    static U32_t constexpr bufferSize = 1U << 10;
    static unsigned char                       poolBuffer[bufferSize];
    static std::pmr::monotonic_buffer_resource poolUpstream{ poolBuffer,
                                                             bufferSize };

    static std::pmr::unsynchronized_pool_resource g_memoryPool{
        std::pmr::pool_options{
          .max_blocks_per_chunk        = 16,
          .largest_required_pool_block = 256,
        },
        &poolUpstream
    };
    return &g_memoryPool;
}

std::pmr::monotonic_buffer_resource *getscratchBuffer()
{
    static U32_t constexpr bufferSize = 4096;
    static unsigned char scratchBuffer[bufferSize * bufferSize];
    static std::pmr::monotonic_buffer_resource g_scratchBuffer{ scratchBuffer,
                                                                bufferSize };
    return &g_scratchBuffer;
}

} // namespace cge
