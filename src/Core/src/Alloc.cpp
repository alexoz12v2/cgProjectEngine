#include "Alloc.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>

namespace cge
{

U64_t alignAddress(U64_t addr, U64_t align)
{
    U64_t const mask = align - 1;
    // TODO substitute
    assert((align & mask) == 0); // pwr of 2
    return (addr + mask) & ~mask;
}

// Aligned allocation function. IMPORTANT: 'align'
// must be a power of 2 (typically 4, 8 or 16).
Byte_t* allocateAligned(U64_t bytes, U64_t align)
{
    // Allocate 'align' more bytes than we need.
    U64_t actualBytes = bytes + align;

    // Allocate unaligned block.
    Byte_t* pRawMem = new Byte_t[actualBytes];

    // Align the block. If no alignment occurred,
    // shift it up the full 'align' bytes so we
    // always have room to store the shift.
    Byte_t* pAlignedMem = alignPointer(pRawMem, align);
    if (pAlignedMem == pRawMem) { pAlignedMem += align; }

    // Determine the shift, and store it.
    // (This works for up to 256-byte alignment.)
    ptrdiff_t shift = pAlignedMem - pRawMem;
    assert(shift > 0 && shift <= 256);

    pAlignedMem[-1] = static_cast<Byte_t>(shift & 0xFF);
    return pAlignedMem;
}

void freeAligned(Byte_t* CGE_restrict pMem)
{
    if (pMem)
    {
        // Convert to Byte_t pointer.
        Byte_t* pAlignedMem = reinterpret_cast<Byte_t*>(pMem);

        // Extract the shift.
        ptrdiff_t shift = static_cast<ptrdiff_t>(pAlignedMem[-1]);
        if (shift == 0) { shift = 256; }

        // Back up to the actual allocated address,
        // and array-delete it.
        Byte_t* pRawMem = pAlignedMem - shift;
        delete[] pRawMem;
    }
}

#define MEM_TAG(ptr) (((U64_t)ptr) & (0x8000'0000'0000'0000))
#define MEM_PTR(ptr) (((U64_t)ptr) & (0x7fff'ffff'ffff'ffff))

// TODO: insert custom logging + memory tracing
EErr_t Memory_s::init(U64_t size, U64_t align)
{
    m_ptr = allocateAligned(size, align);
    if (m_ptr == nullptr) { return EErr_t::eMemory; }

    m_size = size;
    return EErr_t::eSuccess;
}

Byte_t* Memory_s::atOffset(U64_t offset) const { return m_ptr + offset; }

Memory_s::~Memory_s() noexcept { freeAligned(m_ptr); }

void DoubleBuffer_t::init(Byte_t* CGE_restrict pMemory, U32_t size)
{
    m_ptr       = pMemory;
    m_offset[0] = 0;
    m_offset[1] = size >> 1;
    m_bits      = size << 1;
}

EErr_t DoubleBuffer_t::pushOnCurrent(Byte_t* CGE_restrict pMem, U64_t sizeAlign)
{
    U32_t const current = extractCurrent();
    return push(pMem, sizeAlign, current);
}

EErr_t DoubleBuffer_t::pushOnNext(Byte_t* CGE_restrict pMem, U64_t sizeAlign)
{
    U32_t const next = extractNext();
    return push(pMem, sizeAlign, next);
}

CGE_forceinline EErr_t DoubleBuffer_t::push(
  Byte_t* CGE_restrict pMem,
  U64_t                sizeAlign,
  U32_t                stackIndex)
{
    assert(stackIndex == 0 || stackIndex == 1);
    Byte_t* pCurrentMarker = m_ptr + m_offset[stackIndex];

    U32_t const align = sizeAlign & 0xffff'ffff;
    U32_t const size  = sizeAlign >> 32u;

    // align
    Byte_t*    pAlignedPtr = alignPointer(pCurrentMarker, align);
    auto const offset      = static_cast<U32_t>(pAlignedPtr - pCurrentMarker);

    // need to store a 1 byte wide offset in all cases for the free routine,
    // therefore add the full alignment
    if (pAlignedPtr == pCurrentMarker) { pAlignedPtr += offset; }

    // Determine the shift, and store it.
    // (This works for up to 256-byte alignment.)
    auto const shift = static_cast<U32_t>(pAlignedPtr - pCurrentMarker);
    assert(shift > 0 && shift <= 256); // TODO substitute assertion

    pAlignedPtr[-1] = static_cast<Byte_t>(shift & 0xFF);

    assert(
      size < extractSize() / 2
      && pAlignedPtr + size <= m_ptr + (size * stackIndex) / 2);
    std::memcpy(pAlignedPtr, pMem, size);
    m_offset[stackIndex] += size + shift;

    return EErr_t::eSuccess;
}

void DoubleBuffer_t::clearCurrent() { m_offset[extractCurrent()] = 0; }

void DoubleBuffer_t::clearNext() { m_offset[extractNext()] = 0; }

void PoolDBG_t::init(Byte_t* CGE_restrict pMemory, U64_t size)
{
    // assert it is a power of 2
    assert((size & (size - 1)) == 0);

    // assert it is a power 2^k, where k >= 20
    assert(size >> 19 != 0);

    // compute scratchpad size and index size
    m_indexSize      = size >> 13; // 32 kiB if size is 256 MiB
    m_scratchPadSize = size >> 15; // 8  kiB if size is 256 MiB
    assert(m_indexSize != 0 && m_scratchPadSize != 0);

    m_ptr  = pMemory;
    m_size = size;

    // initialize all chunks in the index as nullptr
    auto const node = IndexNode_t{ .chunk = nullptr, .bits = 0 };
    for (U32_t i = 0; i != m_indexSize; i += sizeof(IndexNode_t))
    {
        std::memcpy(m_ptr + i, &node, sizeof(node));
    }
}

void PoolDBG_t::scratchPad(Byte_t** pOut, U32_t* pOutSize)
{
    *pOut     = m_ptr + m_indexSize;
    *pOutSize = m_scratchPadSize;
}

U32_t PoolDBG_t::chooseChunkSize([[maybe_unused]] U64_t objSize) const
{
    // decrement to handle case in which power of 2
    objSize--;

    // set all previous bits
    objSize |= (objSize >> 1);
    objSize |= (objSize >> 2);
    objSize |= (objSize >> 4);
    objSize |= (objSize >> 8);
    objSize |= (objSize >> 16);
    objSize |= (objSize >> 32);

    // increment to get the power of 2 and return (cast as we don't expect large
    // allocations)
    ++objSize;

    // we want to contain 64 of such objects in a chunk
    objSize <<= 6;

    return static_cast<U32_t>(objSize);
}

U32_t PoolDBG_t::indexCount() const
{
    return m_indexSize / sizeof(IndexNode_t);
}


void StackBuffer_t::init(Byte_t* CGE_restrict pMemory, U32_t size)
{
    m_pBase  = pMemory;
    m_offset = 0;
    m_size   = size;
}

EErr_t StackBuffer_t::push(Byte_t* CGE_restrict pMemory, U64_t sizeAlign)
{
    U32_t const size  = static_cast<U32_t>(sizeAlign >> 32u);
    U32_t const align = static_cast<U32_t>(sizeAlign & 0x0000'0000'ffff'ffff);
    Byte_t*     pCurrentMarker = m_pBase + m_offset;
    assert(pCurrentMarker + align + size <= m_pBase + m_size); // TODO

    Byte_t*    pAlignedPtr = alignPointer(pCurrentMarker, align);
    auto const offset      = static_cast<U32_t>(pAlignedPtr - pCurrentMarker);

    // need to store a 1 byte wide offset in all cases for the free routine,
    // therefore add the full alignment
    if (pAlignedPtr == pCurrentMarker) { pAlignedPtr += offset; }

    // Determine the shift, and store it.
    // (This works for up to 256-byte alignment.)
    auto const shift = static_cast<U32_t>(pAlignedPtr - pCurrentMarker);
    assert(shift > 0 && shift <= 256); // TODO substitute assertion

    pAlignedPtr[-1] = static_cast<Byte_t>(shift & 0xFF);

    std::memcpy(pAlignedPtr, pMemory, size);
    m_offset += size + shift;

    return EErr_t::eSuccess;
}

void StackBuffer_t::clear() { m_offset = 0; }

// TODO: delete once not necessary
namespace detail
{
    void poolDeleter(Byte_t* ptr) { free(ptr); }
} // namespace detail
void Pool_t::free(Byte_t* CGE_restrict pObj, Sid_t blkId)
{
    // find the index of this pointer in the vector
    auto it = std::ranges::find_if(
      m_ptrs,
      [pObj, blkId](std::pair<Sid_t const, AllocTuple_t> const& keyVal) -> bool
      {
          auto const& tuple = keyVal.second;
          U32_t const disp  = tuple.spec.size;

          if (tuple.spec.count == 1)
          {
              return pObj == tuple.ptr.get() && blkId == tuple.spec.tag;
          }
          for (U32_t i = 0; i != tuple.spec.count; ++i)
          {
              if (tuple.ptr.get() + i * disp == pObj && blkId == tuple.spec.tag)
              {
                  return true;
              }
          }

          return false;
      });

    // if you could't find it, do nothing
    if (it == m_ptrs.cend()) { return; }

    // possibly execute the destructor
    auto const& tuple = it->second;
    if (tuple.destroy)
    {
        U32_t const disp = tuple.spec.size;
        auto const& ptr  = tuple.ptr.get();

        if (tuple.spec.count == 1) { tuple.destroy(ptr); }
        else
        {
            for (U32_t i = 0; i != tuple.spec.count; ++i)
            {
                tuple.destroy(tuple.ptr.get() + i * disp);
            }
        }
    }

    // remove corresponding element in the map
    m_ptrs.erase(it);
}
} // namespace cge
