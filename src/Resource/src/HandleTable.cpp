
#include "HandleTable.h"
#include "Core/Alloc.h"

#include <cassert>

namespace cge
{

HandleTable_t g_handleTable;

void onRefDestroy(Byte_t* CGE_restrict hTabFinger)
{
    g_handleTable.decCnt(hTabFinger);
}

bool onRefWriteAccess(
  Byte_t* CGE_restrict hTabFinger,
  U32_t* CGE_restrict  outLock)
{
    return g_handleTable.tryLock(hTabFinger, outLock);
}

void onRefCopy(Byte_t* CGE_restrict hTabFinger)
{
    g_handleTable.incCnt(hTabFinger);
}

void onRefRelease(Byte_t* CGE_restrict hTabFinger, U32_t* CGE_restrict outLock)
{
    g_handleTable.release(hTabFinger, outLock);
}

EErr_t HandleTable_t::init(U32_t capacityPerPoolBytes)
{
    EErr_t error = EErr_t::eSuccess;
    // adjust capacityPerPoolBytes and round it up to a multiple of
    // sizeof(Entry_t)
    U32_t count          = (capacityPerPoolBytes / Entry_t::size) + 1;
    capacityPerPoolBytes = count * Entry_t::size;

    Entry_t** pPtr = &m_ptrs[0];
    // try to allocate
    PoolAllocationSpec_t spec{ .tag       = HANDLETABLE_SID,
                               .alignment = alignof(Entry_t),
                               .size      = sizeof(Entry_t),
                               .count     = count };
    error = g_pool.allocateN<Entry_t>(spec, (Byte_t**)pPtr);

    // if ok, then zero out memory, otherwise return status
    if (error != EErr_t::eSuccess) { return error; }
    m_sizes[0]        = 0;
    m_capacityPerPool = capacityPerPoolBytes;
    m_poolCount       = 1;
    m_currentLock     = initialLock;

    U8_t* ptr = (U8_t*)m_ptrs[0];
    for (U32_t i = 0; i != m_capacityPerPool; ++i) { ptr[i] = 0; }

    return error;
}

EErr_t HandleTable_t::incCnt(Byte_t* CGE_restrict hTabFinger) const
{
    Entry_t* pEntry = (Entry_t*)hTabFinger;
    assert(!pEntry->free() && !pEntry->tomb());
    ++(pEntry->f.cnt);

    return EErr_t::eSuccess;
}

void HandleTable_t::decCnt(Byte_t* CGE_restrict hTabFinger)
{
    Entry_t* pEntry = (Entry_t*)hTabFinger;
    assert(!pEntry->free() && !pEntry->tomb());
    if (!--(pEntry->f.cnt))
    {
        U32_t lock;
        while (!tryLock(hTabFinger, &lock)) {}

        // is it still zero?
        if (!pEntry->f.cnt)
        {
            // destroy
            pEntry->kill();
            assert(pEntry->tomb());
        }
    }
}

bool HandleTable_t::tryLock(
  Byte_t* CGE_restrict hTabFinger,
  U32_t* CGE_restrict  outLock)
{
    Entry_t* pEntry = (Entry_t*)hTabFinger;
    if (pEntry->f.cnt & Entry_t::occupiedMask) { return false; }

    U32_t expected = pEntry->f.cnt;
    U32_t occupied = expected | Entry_t::occupiedMask;
    bool  cmpxchg  = pEntry->f.cnt.compare_exchange_strong(
      expected, occupied, std::memory_order_acquire);
    if (!cmpxchg) { return false; }

    *outLock = m_currentLock++;
    return true;
}

void HandleTable_t::release(
  Byte_t* CGE_restrict hTabFinger,
  U32_t* CGE_restrict  outLock)
{
    auto* pEntry    = (Entry_t*)hTabFinger;
    U32_t expected  = pEntry->f.cnt;
    U32_t available = expected & (~Entry_t::occupiedMask);
    pEntry->f.cnt.compare_exchange_strong(
      expected, available, std::memory_order_acquire);
    *outLock = unlocked;
}

U32_t HandleTable_t::checkAvailable(U32_t poolIndex, U32_t position)
{
    assert(poolIndex < m_poolCount);

    // for each entry, check if there's a free entry
    U32_t    i       = position;
    Entry_t* ptr     = m_ptrs[poolIndex];
    U32_t    freePos = npos;

    do {
        if (ptr[i].free() || ptr[i].tombstone)
        {
            freePos = i;
            break;
        }

        i = (i + 1) % m_capacityPerPool;
    } while (i != position);

    return freePos;
}

EErr_t HandleTable_t::insertInNewPool(Sid_t sid, Byte_t* ptr)
{
    Entry_t** pPtr = &m_ptrs[m_poolCount];
    if (m_poolCount >= MAX_POOL_COUNT) { return EErr_t::eMemory; }

    PoolAllocationSpec_t spec{ .tag       = HANDLETABLE_SID,
                               .alignment = alignof(Entry_t),
                               .size      = sizeof(Entry_t),
                               .count     = m_poolCount };

    EErr_t status = g_pool.allocateN(spec, (Byte_t**)pPtr);

    if (status != EErr_t::eSuccess) { return status; }

    Entry_t* pPool       = m_ptrs[m_poolCount];
    m_sizes[m_poolCount] = 1;
    m_poolCount++;

    auto byteptr = (U8_t*)pPool;
    for (U32_t i = 0; i != m_capacityPerPool; ++i) { byteptr[i] = 0; }

    pPool[0].f.sid = sid;
    pPool[0].f.ptr = ptr;
    pPool[0].f.cnt = 1;

    return status;
}
} // namespace cge
