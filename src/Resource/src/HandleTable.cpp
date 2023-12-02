
#include "HandleTable.h"
#include "Core/Alloc.h"

#include <cassert>
#include <functional> // std::hash
// TODO atomic operations

HandleTable_t g_HandleTable;

void onRefDestroy(Byte_t* CGE_restrict hTabFinger)
{
    g_HandleTable.decCnt(hTabFinger);
}

bool onRefWriteAccess(
  Byte_t* CGE_restrict hTabFinger,
  U32_t* CGE_restrict  outLock)
{
    return g_HandleTable.tryLock(hTabFinger, outLock);
}

void onRefCopy(Byte_t* CGE_restrict hTabFinger)
{
    g_handleTable.incCnt(hTabFinger);
}

void onRefRelease(Byte_t* CGE_restrict hTabFinger, U32_t* CGE_restrict outLock)
{
    g_handleTable.release(hTabFinger, outLock);
}

template<> struct std::hash<Sid_t>
{
    std::size_t operator()(Sid_t s) const noexcept
    {
        std::size_t h = std::hash<U64_t>{}(s.id);
        return h;
    }
};

EErr_t HandleTable_t::init(U32_t capacityPerPoolBytes)
{
    EErr_t error = EErr_t::eSuccess;
    // adjust capacityPerPoolBytes and round it up to a multiple of
    // sizeof(Entry_t)
    capacityPerPoolBytes =
      ((capacityPerPoolBytes / Entry_t::size) + 1) * Entry_t::size;

    Entry_t** pPtr = &m_ptrs[0];
    // try to allocate
    error = g_pool.allocateN(pPtr, capacityPerPoolBytes, HANDLETABLE_SID);

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

template<typename T>
EErr_t HandleTable_t::insert(Sid_t sid, Byte_t* ptr, Ref<T>* outRef)
{
    static std::hash<Sid_t> constexpr hasher;

    EErr_t status            = EErr_t::eSuccess;
    U32_t  position          = hasher(sid) % m_capacityPerPool;
    U32_t  availablePosition = npos;
    U32_t  availablePool     = m_poolCount;

    // find available entry in one of current pool, checking them in decreasing
    // order
    for (U32_t i = m_poolCount - 1; i < m_poolCount; --i)
    {
        availablePosition = checkAvailable(i, position);
        if (availablePosition != npos)
        {
            availablePool = i;
            // if we have the position, place the entry there and return
            Entry_t* curptr = m_ptrs[availablePool];

            U32_t expected = 0;
            bool cmpxchg = ptr[availablePosition].f.cnt.compare_exchange_strong(
              expected, 1, std::memory_order_acquire);

            if (expected)
            {
                curptr[availablePosition].f.sid = sid;
                curptr[availablePosition].f.ptr = ptr;
                ++m_sizes[availablePool];
                break;
            }
        }
    }

    if (availablePosition == npos)
    {
        if (m_poolCount < MAX_POOL_COUNT)
        {
            // if we don't have a position, and we have space left, allocate a
            // new pool
            status = insertInNewPool(sid, ptr);
        }
        else
        {
            // if we don't have a position, and there's no space left, error
            status = EErr_t::eMemory;
        }
    }

    outRef->sid   = sid;
    outRef->m_ptr = ptr;
    return status;
}

template<typename T> WeakRef_t<T> HandleTable_t::find(Sid_t sid) const
{
    static std::hash<Sid_t> constexpr hasher;

    for (U32_t i = 0; i != m_poolCount; ++i)
    {
        Entry_t* ptr     = m_ptrs[i];
        U32_t    initial = hasher(sid);

        U32_t j = initial;
        do {
            if (sid.id == ptr[j].f.sid.id) { ++ptr[j].f.cnt; }
        } while (j != initial);
    }
}

EErr_t HandleTable_t::incCnt(Byte_t* CGE_restrict hTabFinger) const
{
    Entry_t* pEntry = (Entry_t*)hTabFinger;
    assert(!pEntry->free() && !pEntry->tomb());
    ++(pEntry->f.cnt);
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
    if (m_poolCount >= MAX_POOL_COUNT) { return EErr_t::eMemory; }

    Entry_t** pPtr = &m_ptrs[m_poolCount];
    EErr_t status  = g_pool.allocateN(pPtr, m_capacityPerPool, HANDLETABLE_SID);

    if (status != EErr_t::eSuccess) { return status; }

    Entry_t* pPool       = m_ptrs[m_poolCount];
    m_sizes[m_poolCount] = 1;
    m_poolCount++;

    U8_t* byteptr = (U8_t*)pPool;
    for (U32_t i = 0; i != m_capacityPerPool; ++i) { byteptr[i] = 0; }

    pPool[0].f.sid = sid;
    pPool[0].f.ptr = ptr;
    pPool[0].f.cnt = 1;

    return status;
}
