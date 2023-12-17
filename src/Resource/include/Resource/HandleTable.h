#pragma once

#include "Core/Containers.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Resource/Ref.h"
#include "Resource/WeakRef.h"

#include <atomic>

namespace cge
{

class HandleTable_t
{
  public:
    static U32_t constexpr MAX_POOL_COUNT  = 16;
    static Sid_t constexpr HANDLETABLE_SID = "HandleTable_t"_sid;
    static U32_t constexpr npos            = -1;
    static U32_t constexpr unlocked        = ~0; // such that works with tomb()
    static U32_t constexpr initialLock = 1; // not zero, no confusion with free

    /** uses g_pool */
    EErr_t init(U32_t capacityPerPoolBytes);
    EErr_t incCnt(Byte_t* CGE_restrict hTabFinger) const;
    void   decCnt(Byte_t* CGE_restrict hTabFinger);
    bool tryLock(Byte_t* CGE_restrict hTabFinger, U32_t* CGE_restrict outLock);
    void release(Byte_t* CGE_restrict hTabFinger, U32_t* CGE_restrict outLock);

    template<typename T> WeakRef_t<T> find(Sid_t sid) const
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
    template<typename T>
    EErr_t insert(Sid_t sid, Byte_t* ptr, ::cge::Ref_s<T>* outRef)
    {
        static std::hash<Sid_t> constexpr hasher;

        EErr_t status            = EErr_t::eSuccess;
        U32_t  position          = hasher(sid) % m_capacityPerPool;
        U32_t  availablePosition = npos;
        U32_t  availablePool     = m_poolCount;

        // find available entry in one of current pool, checking them in
        // decreasing order
        for (U32_t i = m_poolCount - 1; i < m_poolCount; --i)
        {
            availablePosition = checkAvailable(i, position);
            if (availablePosition != npos)
            {
                availablePool = i;
                // if we have the position, place the entry there and return
                Entry_t* curptr = m_ptrs[availablePool];

                U32_t expected = 0;
                bool  cmpxchg  = ((Entry_t*)ptr)[availablePosition]
                                 .f.cnt.compare_exchange_strong(
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
                // if we don't have a position, and we have space left, allocate
                // a new pool
                status = insertInNewPool(sid, ptr);
            }
            else
            {
                // if we don't have a position, and there's no space left, error
                status = EErr_t::eMemory;
            }
        }

        outRef->m_sid = sid;
        outRef->m_ptr = (T*)ptr;
        return status;
    }

  private:
    union Entry_t
    {
        static U32_t constexpr occupiedMask = 0x0100'0000u;
        // 256 bits in debug mode, 160 bits otherwise
        struct Filled_t
        {
            Sid_t              sid;
            Byte_t*            ptr;
            std::atomic<U32_t> cnt; // low 24 bits as ref cnt, high 8 for flags.
                                    // |*|*|*|*|*|*|*|occupied|
            U32_t lock = unlocked;
        };
        Filled_t f;
        Byte_t   zero[sizeof(Filled_t)];
        Byte_t   tombstone[sizeof(Filled_t)];

        void kill()
        {
            Byte_t tombstone[sizeof(Filled_t)]{ 0xff };
            std::memcpy(this, &tombstone, sizeof(Filled_t));
        }

#if defined(CGE_DEBUG)
        static U32_t constexpr size = 32;
        bool free()
        {
            auto const tmp = (U32_t*)zero;
            return !(
              tmp[0] || tmp[1] || tmp[2] || tmp[3] || tmp[4] || tmp[5]
              || tmp[6]);
        }
        bool tomb()
        {
            auto const tmp = (U32_t*)tombstone;
            return tmp[0] == 0xffff'ffffu && tmp[1] == 0xffff'ffffu
                   && tmp[2] == 0xffff'ffffu && tmp[3] == 0xffff'ffffu
                   && tmp[4] == 0xffff'ffffu && tmp[5] == 0xffff'ffffu
                   && tmp[6] == 0xffff'ffffu;
        }
#else
        static U32_t constexpr size = 20;
        bool free()
        {
            auto const tmp = (U64_t*)zero;
            return !(tmp[0] || tmp[1]);
        }
        bool tomb()
        {
            auto const tmp = (U64_t*)zero;
            return tmp[0] == 0xffff'ffff'ffff'ffffULL
                   && tmp[1] == 0xffff'ffff'ffff'ffffULL;
        }
#endif
    };
    static_assert(sizeof(Entry_t) == Entry_t::size);


    U32_t  checkAvailable(U32_t poolIndex, U32_t position);
    EErr_t insertInNewPool(Sid_t sid, Byte_t* ptr);

    Array<Entry_t*, MAX_POOL_COUNT> m_ptrs;
    Array<U32_t, MAX_POOL_COUNT>    m_sizes;
    U32_t                           m_capacityPerPool;
    U32_t                           m_poolCount;
    U32_t                           m_currentLock;
};

extern HandleTable_t g_handleTable;
} // namespace cge
