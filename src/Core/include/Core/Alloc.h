#pragma once

#include "Core/StringUtils.h"
#include "Core/Type.h"

#include <gsl/pointers>

// TODO remove
#include <cassert>
#include <cstdio>

#define CGE_DOUBLEBUFFER_SIZEALIGN(size, align) (((size) << 32) | (align))

/** @file Alloc.h
 *  @brief contains declarations of allocators DoubleBuffer_t, PoolDBG_t and
 * routines for the 1GB Huge Page allocation File Sections:
 */

namespace cge
{

U64_t alignAddress(U64_t addr, U64_t align);

template<typename T>
CGE_forceinline T *alignPointer(T *CGE_restrict ptr, U64_t align)
{
    U64_t const addr        = reinterpret_cast<U64_t>(ptr);
    U64_t const addrAligned = alignAddress(addr, align);
    return reinterpret_cast<T *>(addrAligned);
}

/** @class Memory_s.
 *  @brief Represents a System Memory resource. The Application will have
 * available 1GB of memory, whose allocation is performed by the @ref init()
 * function. Follows RAII principle, therefore releases the allocated memory at
 * destruction time.
 */
struct Memory_s
{
  public:
    Memory_s() = default;
    Memory_s(Byte_t *CGE_restrict pMemory, U64_t size)
      : m_ptr(pMemory), m_size(size)
    {
    }
    Memory_s(Memory_s const &other)                = default;
    Memory_s(Memory_s &&other) noexcept            = delete;
    Memory_s &operator=(Memory_s const &other)     = default;
    Memory_s &operator=(Memory_s &&other) noexcept = delete;
    ~Memory_s() noexcept;

    /** @fn init.
     *  @brief tries to allocate the memory resource. Rounds up size to nearest
     * power of 2
     *  @param size size of the memory resouce to allocate
     *  @param align desired alignment of the memory resource
     *  @return eSuccess if memory was successfully allocated, eMemory otherwise
     */
    EErr_t init(U64_t size, U64_t align);

    /** @fn atOffset
     *  @brief retrieves a pointer to the specified offset
     *  @param offset in bytes added to the base pointer to the memory resource
     *  @return pointer to a memory resource offset by given parameter
     *  @warning no overlap or bound check is performed. Memory_s won't keep
     * track of used memory
     */
    Byte_t *atOffset(U64_t offset) const;

  private:
    gsl::owner<Byte_t *> m_ptr =
      nullptr; // pointer is tagged: Most Sig Bit == 1 -> page alloc
    U64_t m_size = 0;
};

class StackBuffer_t
{
  public:
    void   init(Byte_t *CGE_restrict pMemory, U32_t size);
    EErr_t push(Byte_t *CGE_restrict pMemory, U64_t sizeAlign);
    void   clear();

  private:
    Byte_t *m_pBase;
    U32_t   m_size;
    U32_t   m_offset;
};

/** @class DoubleBuffer_t.
 *  @brief Double Buffer Stack Allocator which manages a block of memory passed
 * in input, as 2 stacks by splitting it in its midpoint. Both stacks are placed
 * in the lowest address and grow toward incresing addresses
 */
class DoubleBuffer_t
{
  public:
    /** @fn init
     *  @brief sets up base pointer and offsets from the given memory resource
     * and its size
     *  @param pMemory memory resource on which to construct a double stack
     * allocator
     *  @param size range of pMemory which the double buffer will take
     *  @warning pMemory is supposed to be a pointer to a memory resource whose
     * lifetime exceeds the lifetime of the DoubleBuffer instance
     */
    void init(Byte_t *CGE_restrict pMemory, U32_t size);


    /** @fn pushOnCurrent
     *  @brief pushes element passed by pointer on the current stack
     *  @param pMem      element to push onto the current stack
     *  @param sizeAlign [63:32] = size of element, [31:0] = desired alignment
     *  @returns error when out of bounds ...
     *  @warning ONLY if either debug mode or bounds check enabled
     * (CGE_DOUBLEBUFFER_BOUNDS_CHECK)
     *  @note As it takes a pointer to memory, it requires the object in
     * question being trivially constructible
     */
    EErr_t pushOnCurrent(Byte_t *CGE_restrict pMem, U64_t sizeAlign);

    /** @fn pushOnNext.
     *  @brief see @ref pushOnCurrent
     */
    EErr_t pushOnNext(Byte_t *CGE_restrict pMem, U64_t sizeAlign);

    /** @warning they Suppose trivially destructible objects (for now) */
    void clearCurrent();
    void clearNext();

    /// @note no need for a destructor as the memory resource outlasts the
    /// DoubleBuffer
    // ~DoubleBuffer_t();

  private:
    U32_t extractCurrent() const { return m_bits & 1u; }
    U32_t extractNext() const { return (~m_bits) & 1u; }
    U32_t extractSize() const { return m_bits >> 1u; }

    CGE_forceinline EErr_t
      push(Byte_t *CGE_restrict pMem, U64_t sizeAlign, U32_t stackIndex);

    Byte_t *m_ptr;
    U32_t   m_offset[2];
    U32_t   m_bits; // U31_t   m_size;
                    // U1_t    m_curStack;
};

/**
 *  temporary substitution.
 */
namespace detail
{
    void poolDeleter(Byte_t *ptr);
}
struct PoolAllocationSpec_t
{
    Sid_t tag;
    U32_t alignment;
    U32_t size;
    U32_t count; // ignored if allocation function allocateds only 1
};
class Pool_t
{
  public:
    void init(Byte_t *CGE_restrict pMemory, U64_t size)
    {
        m_ptrs.reserve(1024);
    }

    template<typename T = void>
    EErr_t
      allocateN(PoolAllocationSpec_t const &spec, Byte_t **CGE_restrict ppOut)
    {
        *ppOut =
          (Byte_t *)_aligned_malloc(spec.size * spec.count, spec.alignment);
        if (!*ppOut) { return EErr_t::eMemory; }
        auto uptr = std::unique_ptr<Byte_t[], decltype(&detail::poolDeleter)>(
          (*ppOut), &detail::poolDeleter);

        if constexpr (
          std::is_same_v<T, void> || std::is_trivially_destructible_v<T>)
        {
            m_ptrs.try_emplace(spec.tag, spec, std::move(uptr), nullptr);
        }
        else
        {
            m_ptrs.try_emplace(
              spec.tag,
              spec,
              std::move(uptr),
              [](void *obj) { ((T *)obj)->~T(); });
        }

        return EErr_t::eSuccess;
    }

    template<typename T = void>
    EErr_t
      allocate(PoolAllocationSpec_t const &spec, Byte_t **CGE_restrict ppOut)
    {
        *ppOut = (Byte_t *)_aligned_malloc(spec.size, spec.alignment);
        if (!*ppOut) { return EErr_t::eMemory; }
        auto localSpec  = spec;
        localSpec.count = 1;

        auto uptr = std::unique_ptr<Byte_t[], decltype(&detail::poolDeleter)>(
          *ppOut, &detail::poolDeleter);

        if constexpr (
          std::is_same_v<T, void> || std::is_trivially_destructible_v<T>)
        {
            m_ptrs.try_emplace(
              localSpec.tag, localSpec, std::move(uptr), nullptr);
        }
        else
        {
            m_ptrs.try_emplace(
              localSpec.tag,
              localSpec,
              std::move(uptr),
              [](void *obj) { ((T *)obj)->~T(); });
        }

        return EErr_t::eSuccess;
    }
    void free(Byte_t *CGE_restrict pObj, Sid_t blockId);

  private:
    using DestroyFunc_t = void (*)(void *);
    struct AllocTuple_t
    {
        PoolAllocationSpec_t                                      spec;
        std::unique_ptr<Byte_t[], decltype(&detail::poolDeleter)> ptr;
        DestroyFunc_t                                             destroy;
    };
    std::unordered_map<Sid_t, AllocTuple_t> m_ptrs;
};

/** @class PoolDBG_t.
 *  @brief Represents a Pool allocator which is dynamically subdivided to create
 * pools of the requested size on demand
 * @warning BUGGED! Solve it later
 */
class PoolDBG_t
{
    // HAS TO MEMORIZE SOME TAGS ON TYPES, eg if it is a IModule
  public:
    /**
     * @warning pMemory is supposed to be a pointer to a memory resource whose
     * lifetime exceeds the lifetime of the Pool instance
     */
    void init(Byte_t *CGE_restrict pMemory, U64_t size);

    template<typename T>
    EErr_t allocateN(T **CGE_restrict ppOutObj, U64_t count, Sid_t tag)
    {
        // always create new chunk, simplify TODO better
        U32_t const chunkSize = chooseChunkSize(sizeof(T));
        U32_t const index     = indexCount();
        if (indexCount() == m_indexSize) { return EErr_t::eMemory; }

        auto pNewChunk = (ChunkRecord_t *)(poolBuffer() + index * chunkSize);
        // first of the N blocks
        BlockRecord_t *pFirstBlock =
          (BlockRecord_t *)((Byte_t *)pNewChunk + sizeof(BlockRecord_t))
          + sizeof(void *);
        assert((Byte_t *)pFirstBlock != (Byte_t *)pNewChunk);

        T *ptr = (T *)(alignPointer(pFirstBlock, alignof(T)));

        pFirstBlock = (BlockRecord_t *)((Byte_t *)ptr - sizeof(BlockRecord_t));

        U64_t blockSize = ((Byte_t *)ptr + sizeof(T)) - (Byte_t *)pFirstBlock;
        assert(blockSize >= sizeof(T) + sizeof(BlockRecord_t));
        assert(
          (Byte_t *)(pNewChunk) + sizeof(BlockRecord_t)
          <= (Byte_t *)pFirstBlock);
        U32_t blockCount =
          (chunkSize - sizeof(BlockRecord_t))
          / blockSize; // account for initial chunck control block
        assert(blockCount >= count);
        assert(blockCount * blockSize <= chunkSize - sizeof(BlockRecord_t));

        // fill each of the N structures
        auto pNthBlock = pFirstBlock;
        for (U32_t i = 0; i != count; ++i)
        {
            pNthBlock->block.magic = BlockRecord_t::getMagic();
            pNthBlock->block.index = i;
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                pNthBlock->block.destroy = [](void const *obj)
                { static_cast<T const *>(obj)->~T(); };
            }
            else
            {
                pNthBlock->block.destroy = [](void const *obj) {};
            }

            pNthBlock = (BlockRecord_t *)(((Byte_t *)pNthBlock) + blockSize);
        }

        // wire up remaining
        auto pEndChunk   = (BlockRecord_t *)(((Byte_t *)pNewChunk) + chunkSize);
        auto pNthp1Block = (BlockRecord_t *)(((Byte_t *)pNthBlock) + blockSize);
        pNthBlock->nextFree   = pNthp1Block;
        pNthp1Block->nextFree = nullptr;
        pFirstBlock->nextFree = pNthBlock;
        while (((Byte_t *)pNthp1Block) + blockSize < (Byte_t *)pEndChunk)
        {
            pNthBlock = pNthp1Block;
            pNthp1Block =
              (BlockRecord_t *)(((Byte_t *)pNthp1Block) + blockSize);
            pNthBlock->nextFree = pNthp1Block;
        }

        pNthp1Block->nextFree = nullptr;
        return EErr_t::eSuccess;
    }

    // TODO refactor to common, untempletized, method
    template<typename T>
    EErr_t allocate(T **CGE_restrict ppOutObj, Sid_t tag, bool bMatchTag)
    {
        // choose a chunk size depending on the size of the object
        U32_t const chunkSize = chooseChunkSize(sizeof(T));
        EErr_t      status    = EErr_t::eSuccess;
        printf(
          "object of %zu size, chusen chunk size %u\n", sizeof(T), chunkSize);

        assert(chunkSize >= sizeof(T));

        // find in the index a chunk with objSize == sizeof(T)
        auto const     index       = reinterpret_cast<IndexNode_t *>(m_ptr);
        ChunkRecord_t *pChunkFound = nullptr;

        printf("first index's chunk pointer: %p\n", index[0].chunk);

        U32_t i = 0; // will be recorded in the ChunkRecord_t as index
        for (; i != indexCount() && index[i].chunk != nullptr; ++i)
        {
            if (
              sizeof(T) == index[i].objSize()
              && alignof(T) == index[i].objAlign() && !index[i].chunkFull()
              && (!bMatchTag || (index[i].chunk->tag == tag.id)))
            {
                pChunkFound = index[i].chunk;
                break;
            }
        }

        printf("final chunk index: %u\n", i);

        if (pChunkFound)
        {
            // navigate to the first free object, rewire previous ChunkRecord_t
            // to next ChunkRecord_t
            BlockRecord_t *const pFirstFree = pChunkFound->nextFree;
            BlockRecord_t *const pNextFree  = pFirstFree->nextFree;

            // update pointers
            pChunkFound->nextFree = pNextFree;

            // tag record as occupied
            pFirstFree->block.index   = i;
            pFirstFree->block.magic   = BlockRecord_t::getMagic();
            pFirstFree->block.destroy = [](void const *obj)
            { static_cast<T const *>(obj)->~T(); };

            // BlockRecords should be 16 byte aligned, therefore there should be
            // no need to align ptr (I think)
            auto ptr =
              reinterpret_cast<T *>(pFirstFree + sizeof(ChunkRecord_t));

            *ppOutObj = ptr;
        }
        else if (i == indexCount()) // !pChunkFound and full
        {
            // we're out of memory
            status = EErr_t::eMemory;
        }
        else // if we are not out of memory, then i is the first chunk free
        {
            printf("didn't find any index record\n");
            // first chunk record is fixed at the beginning of the chunk, the
            // other ones are placed before the addresses of the avaliable
            // blocks. Therefore in the beginning of a chunk there will be 2
            // adjacent blocks. We need an initial chunk control block
            auto pNewChunk =
              reinterpret_cast<ChunkRecord_t *>(poolBuffer() + i * chunkSize);
            printf(
              "%p start of mem, %p end of mem, %p chosen chunk\n",
              m_ptr,
              m_ptr + m_size,
              pNewChunk);

            BlockRecord_t *pFirstBlock =
              (BlockRecord_t *)((Byte_t *)pNewChunk + sizeof(BlockRecord_t))
              + sizeof(void *);
            assert((Byte_t *)pFirstBlock != (Byte_t *)pNewChunk);

            T *ptr = (T *)(alignPointer(pFirstBlock, alignof(T)));

            pFirstBlock =
              (BlockRecord_t *)((Byte_t *)ptr - sizeof(BlockRecord_t));

            U64_t blockSize =
              ((Byte_t *)ptr + sizeof(T)) - (Byte_t *)pFirstBlock;
            assert(blockSize >= sizeof(T) + sizeof(BlockRecord_t));

            // move back from the object so that we can prepend the control
            // structure
            assert(
              (Byte_t *)(pNewChunk) + sizeof(BlockRecord_t)
              <= (Byte_t *)pFirstBlock);

            // fill pNextChunk structure
            pFirstBlock->block.magic = BlockRecord_t::getMagic();
            pFirstBlock->block.index = i;
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                pFirstBlock->block.destroy = [](void const *obj)
                { static_cast<T const *>(obj)->~T(); };
            }
            else
            {
                pFirstBlock->block.destroy = [](void const *obj) {};
            }

            // TODO optimize. Maybe bitwise ops, like xor + count trailing zeros
            // + shift.
            U32_t blockCount =
              (chunkSize - sizeof(BlockRecord_t))
              / blockSize; // account for initial chunck control block
            assert(blockCount > 2);
            assert(blockCount * blockSize <= chunkSize - sizeof(BlockRecord_t));

            // wire all the blocks together with a singly linked list
            BlockRecord_t *const pEndChunk =
              (BlockRecord_t *)((Byte_t *)(pNewChunk) + chunkSize);
            BlockRecord_t *pPrevChunk =
              (BlockRecord_t *)((Byte_t *)pFirstBlock + blockSize);
            pNewChunk->nextFree = pPrevChunk;
            for (BlockRecord_t *pCurrChunk = pPrevChunk; pCurrChunk < pEndChunk;
                 pCurrChunk =
                   (BlockRecord_t *)((Byte_t *)(pCurrChunk) + blockSize))
            {
                pPrevChunk->nextFree = pCurrChunk;
                pPrevChunk           = pCurrChunk;
            }

            // put into the last free block a nullptr
            pPrevChunk->nextFree = nullptr;
            void *pEnd           = (Byte_t *)pNewChunk + chunkSize;
            printf(
              "computed end of chunk (without last BlockRecord_t): %p\n"
              "actual end of chunk                               : %p\n",
              pPrevChunk,
              pEnd);
            assert((Byte_t *)pPrevChunk <= pEnd);

            *ppOutObj = ptr;

            // appropriately tag the chunk
            pFirstBlock->block.tag = tag.id;
            pNewChunk->tag         = tag.id;
        }

        return status;
    }

    void free(Byte_t *CGE_restrict pObj)
    {
        // look into the ChunkRecord to check the magic number
        auto pChunk =
          reinterpret_cast<BlockRecord_t *>(pObj - sizeof(BlockRecord_t));
        if (pChunk->block.magic != BlockRecord_t::getMagic()) { return; }

        // look in the index structure at offset specified by pChunk->index to
        // find the first BlockRecord
        auto const index = reinterpret_cast<IndexNode_t *>(m_ptr);

        // find previous and successive chunks to the one to be freed
        ChunkRecord_t *pChunkRecord = index[pChunk->block.index].chunk;
        BlockRecord_t *pPrevChunk   = pChunkRecord->nextFree;
        BlockRecord_t *pCurrChunk = pPrevChunk ? pPrevChunk->nextFree : nullptr;

        while (pCurrChunk != nullptr && pCurrChunk < pChunk)
        {
            pPrevChunk = pCurrChunk;
            pCurrChunk = pCurrChunk->nextFree;
        }

        // execute destructor
        pChunk->block.destroy(pObj);
        pChunk->block.magic = 0;

        pPrevChunk->nextFree = pChunk;
        pChunk->nextFree     = pCurrChunk;

        // reset the full bit
        index[pChunk->block.index].bits &= ((~0ull) >> 1u);
    }

    void scratchPad(Byte_t **CGE_restrict ppOut, U32_t *CGE_restrict pOutSize);

    // LACKS A CLEAR METHOD

    /// @note no need for a destructor as the memory resource outlasts the
    /// PoolDBG_t
    // ~PoolDBG_t();

  private:
    union BlockRecord_t
    {
        static U32_t constexpr getMagic() { return 0xf124'0a6bu; }
        struct Block_t
        {
            void (*destroy)(void const *);
            U64_t tag;
            U32_t magic;
            U16_t index;
        } block;

        BlockRecord_t *nextFree;
    };

    struct ChunkRecord_t
    {
        BlockRecord_t *nextFree;
        U64_t          tag;
    };

    struct IndexNode_t
    {
        ChunkRecord_t *chunk;
        U64_t          bits; // U1_t  is chunk full
                             // U48_t object size
                             // U15_t object alignment
        CGE_forceinline U64_t objSize() const
        {
            return bits & ((~0ull) >> 12u);
        }
        CGE_forceinline U16_t objAlign() const
        {
            return static_cast<U16_t>(bits) & 0x7fff;
        }
        CGE_forceinline bool chunkFull() const
        {
            return static_cast<bool>(bits & ~((~0ull) >> 1u));
        }
    };

    CGE_forceinline Byte_t *poolBuffer() const
    {
        return m_ptr + m_indexSize + m_scratchPadSize;
    }

    U32_t chooseChunkSize(U64_t objSize) const;
    U32_t indexCount() const;

    Byte_t *m_ptr;
    U32_t   m_size;
    U16_t   m_indexSize;
    U16_t   m_scratchPadSize;
};

static_assert(
  sizeof(Memory_s) == 16 && sizeof(DoubleBuffer_t) == 24
    && sizeof(PoolDBG_t) == 16,
  "");
extern Pool_t g_pool;

} // namespace cge
