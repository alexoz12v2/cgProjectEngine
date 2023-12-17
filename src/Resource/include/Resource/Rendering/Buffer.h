#pragma once

#include "Core/Containers.h"
#include "Core/Type.h"

#include <type_traits>

namespace cge
{


enum class ETargetType : U8_t
{
    eFloating,
    eInteger,
    eDouble
};

enum class EAccess : U8_t
{
    eNone      = 0,
    eRead      = 1 << 0,
    eWrite     = 1 << 1,
    eReadWrite = 1 << 0 | 1 << 1
};

std::underlying_type_t<EAccess> operator&(EAccess a, EAccess b);

struct LayoutElement_t
{
    U32_t       type;
    U32_t       count;
    ETargetType targetType;
    bool        normalized;
};

inline U32_t constexpr bufferLayoutSize = 8;
class BufferLayout_s
{
  public:
    void                   push(LayoutElement_t const &le);
    U32_t                  stride() const;
    LayoutElement_t const *pElements(U32_t *pOutNumbers) const;

  private:
    Array<LayoutElement_t, bufferLayoutSize> m_elements;

    U32_t m_end    = 0;
    U32_t m_stride = 0;
};

class BufferMapping_s
{
    friend class Buffer_s;

  public:
    // writes synchronized on unmap, reads immediately synchronized
    BufferMapping_s       &copyToBuffer(void const *pData, U32_t size);
    BufferMapping_s const &copyFromBuffer(void *, U32_t off, U32_t sz);
    void                   unmap() const;

  private:
    void synchronizeWithDevice();

    void *m_ptr    = nullptr;
    U32_t m_id     = 0;
    U32_t m_size   = 0;
    U32_t m_offset = 0;
    I32_t m_access = 0;
    U32_t m_target = 0;

    U32_t m_currentOffset = 0;
    void *m_writeFence    = nullptr;

    BufferMapping_s(
      void *ptr,
      U32_t id,
      U32_t size,
      U32_t offset,
      I32_t access,
      U32_t target);
};

class Buffer_s
{
    friend BufferMapping_s;

  public:
    Buffer_s();
    ~Buffer_s();

    void bind(U32_t target) const;
    void unbind(U32_t target) const;

    U32_t id() const { return m_id; }

    void allocateMutable(U32_t target, U32_t size, U32_t usage) const;

    // note: if you make it mappable it will reside in host visible memory
    // permanently (maybe 2 immutable one readwritable one not may be used as
    // staging buffer and vertex buffer)
    void allocateImmutable(U32_t target, U32_t size, EAccess mapAccess) const;

    void
      transferDataImm(U32_t target, U32_t offset, U32_t size, void const *data);

    // add map persistent function if you need it
    BufferMapping_s
      mmap(U32_t target, U32_t offset, U32_t size, EAccess eRW) const;

    // when you wnat to modify a buffer between 2 renderings, add
    // lock(), set data, unlock() functionality
  protected:
    U32_t getId() const { return m_id; }
    U32_t setId(U32_t id) { return (m_id = id); }

  private:
    explicit Buffer_s(U32_t);

    U32_t m_id = 0;
    // if you need it, add metadata like the size of the buffer
};

class VertexBuffer_s : public Buffer_s
{
  public: // name hiding on purpose
    void bind() const;
    void unbind() const;

    BufferMapping_s mmap(U32_t offset, U32_t size, EAccess eRW) const;

    void allocateMutable(U32_t size) const;
    void allocateImmutable(U32_t size, EAccess mapAccess) const;
};

class IndexBuffer_s : public Buffer_s
{
  public: // name hiding on purpose
    void bind() const;
    void unbind() const;

    BufferMapping_s mmap(U32_t offset, U32_t size, EAccess eRW) const;

    void allocateMutable(U32_t size) const;
    void allocateImmutable(U32_t size, EAccess mapAccess) const;
};

class VertexArray_s
{
  public:
    VertexArray_s();
    ~VertexArray_s();

    void bind() const;
    void unbind() const;

    void addBuffer(VertexBuffer_s const &vb, BufferLayout_s const &bl) const;

  private:
    U32_t m_id = 0;
};
} // namespace cge
