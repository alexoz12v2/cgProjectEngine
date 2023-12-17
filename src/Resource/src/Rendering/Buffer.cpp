
#include "Rendering/Buffer.h"

#include "RenderUtils/GLutils.h"

#include <cassert>
#include <glad/gl.h>

namespace cge
{

U32_t sizeFromGLEnum(U32_t);

void BufferLayout_s::push(LayoutElement_t const &le)
{
    assert(m_end != bufferLayoutSize && "out of space");
    m_elements[m_end++] = le;
    m_stride += sizeFromGLEnum(le.type) * le.count;
}

LayoutElement_t const *BufferLayout_s::pElements(U32_t *pOutNumbers) const
{
    *pOutNumbers = m_end;
    return &(m_elements[0]);
}

U32_t BufferLayout_s::stride() const { return m_stride; }

// vertex buffer ---------------------------------------------------------------

Buffer_s::Buffer_s() { GL_CHECK(glGenBuffers(1, &m_id)); }

Buffer_s::~Buffer_s() { GL_CHECK(glDeleteBuffers(1, &m_id)); }

void Buffer_s::bind(U32_t target) const
{
    GL_CHECK(glBindBuffer(target, m_id));
}

void Buffer_s::unbind(U32_t target) const { GL_CHECK(glBindBuffer(target, 0)); }

void VertexBuffer_s::bind() const
{
    auto id = getId();
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, id));
}

void VertexBuffer_s::unbind() const
{
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void VertexBuffer_s::allocateMutable(U32_t size) const
{
    Buffer_s::allocateMutable(GL_ARRAY_BUFFER, size, GL_STATIC_DRAW);
}
void VertexBuffer_s::allocateImmutable(U32_t size, EAccess mapAccess) const
{
    Buffer_s::allocateImmutable(GL_ARRAY_BUFFER, size, mapAccess);
}

void Buffer_s::allocateMutable(U32_t target, U32_t size, U32_t usage) const
{
    bind(target);
    glBufferData(target, size, nullptr, GL_STATIC_DRAW);
}

void Buffer_s::allocateImmutable(U32_t target, U32_t size, EAccess access) const
{
    bind(target);
    U32_t flags = 0;

    switch (access)
    {
    case EAccess::eNone:
        flags = 0;
        break;
    case EAccess::eReadWrite:
    case EAccess::eWrite:
        flags |= GL_MAP_WRITE_BIT;
        [[fallthrough]];
    case EAccess::eRead:
        flags |= GL_MAP_READ_BIT;
        break;
    default:
        assert(false);
    }

    GL_CHECK(glBufferStorage(target, size, nullptr, flags));
}

void Buffer_s::transferDataImm(
  U32_t       target,
  U32_t       offset,
  U32_t       size,
  void const *data)
{
    GL_CHECK(glBindBuffer(target, m_id));
    GL_CHECK(glBufferSubData(target, offset, size, data));
}

BufferMapping_s &BufferMapping_s::copyToBuffer(void const *pData, U32_t size)
{
    assert((m_size - m_currentOffset) >= size && "insufficient mapped space");

    // synchronize
    synchronizeWithDevice();

    void *ptr = (Byte_t *)m_ptr + m_currentOffset;
    std::memcpy(m_ptr, pData, size);

    m_currentOffset += size;

    return *this;
}

BufferMapping_s const &
  BufferMapping_s::copyFromBuffer(void *pData, U32_t offset, U32_t size)
{
    assert(
      size - offset < size && size - offset <= m_size - m_offset
      && "insufficient mapped space");

    // synchronize
    synchronizeWithDevice();

    void *ptr = (Byte_t *)m_ptr + offset;
    std::memcpy(pData, ptr, size);

    return *this;
}

void BufferMapping_s::synchronizeWithDevice()
{
    if (m_writeFence != nullptr)
    {
        auto   fence = (GLsync)m_writeFence;
        GLenum result;
        GL_CHECK(
          result =
            glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 100000000));
        if (result == GL_WAIT_FAILED) { std::printf("GL_WAIT_FAILED"); }
        glDeleteSync(fence);
        m_writeFence = nullptr;
    }
}

void BufferMapping_s::unmap() const
{
    if (m_access & GL_MAP_WRITE_BIT)
    {
        // flush
        GL_CHECK(glFlushMappedBufferRange(m_target, m_offset, m_size));
    }

    GL_CHECK(glUnmapBuffer(m_target));
}

BufferMapping_s
  Buffer_s::mmap(U32_t target, U32_t offset, U32_t size, EAccess eRW) const
{
    void *ptr    = nullptr;
    auto  access = (eRW & EAccess::eRead) ? GL_MAP_READ_BIT : 0;
    access |= (eRW & EAccess::eWrite) ? GL_MAP_WRITE_BIT : 0;
    access |= GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_UNSYNCHRONIZED_BIT;

    BufferMapping_s mapping = { ptr, m_id, size, offset, access, target };

    bind(target);
    if (eRW & EAccess::eWrite)
    {
        GL_CHECK(
          mapping.m_writeFence =
            (void *)glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
    }
    GL_CHECK(mapping.m_ptr = glMapBufferRange(target, offset, size, access));

    return mapping;
}

BufferMapping_s
  VertexBuffer_s::mmap(U32_t offset, U32_t size, EAccess eRW) const
{
    return Buffer_s::mmap(GL_ARRAY_BUFFER, offset, size, eRW);
}

Buffer_s::Buffer_s(U32_t id) : m_id(id) {}

BufferMapping_s::BufferMapping_s(
  void *ptr,
  U32_t id,
  U32_t size,
  U32_t offset,
  I32_t access,
  U32_t target)
  : m_ptr(ptr), m_id(id), m_size(size), m_offset(offset), m_access(access),
    m_target(target)
{
    m_currentOffset = m_offset;
}

void IndexBuffer_s::bind() const
{
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, getId()));
}
void IndexBuffer_s::unbind() const
{
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

BufferMapping_s IndexBuffer_s::mmap(U32_t offset, U32_t size, EAccess eRW) const
{
    return Buffer_s::mmap(GL_ELEMENT_ARRAY_BUFFER, offset, size, eRW);
}

void IndexBuffer_s::allocateMutable(U32_t size) const
{
    return Buffer_s::allocateMutable(
      GL_ELEMENT_ARRAY_BUFFER, size, GL_STATIC_DRAW);
}
void IndexBuffer_s::allocateImmutable(U32_t size, EAccess mapAccess) const
{
    return Buffer_s::allocateImmutable(
      GL_ELEMENT_ARRAY_BUFFER, size, mapAccess);
}

// vertex Array ---------------------------------------------------------------

VertexArray_s::VertexArray_s() { GL_CHECK(glGenVertexArrays(1, &m_id)); }

VertexArray_s::~VertexArray_s() { GL_CHECK(glDeleteVertexArrays(1, &m_id)); }

void VertexArray_s::bind() const { GL_CHECK(glBindVertexArray(m_id)); }

void VertexArray_s::unbind() const { GL_CHECK(glBindVertexArray(0)); }

void VertexArray_s::addBuffer(
  VertexBuffer_s const &vb,
  BufferLayout_s const &bl) const
{
    U32_t      elementCount;
    auto const pElements = bl.pElements(&elementCount);
    U32_t      offset    = 0;

    vb.bind();

    for (U32_t i = 0; i != elementCount; ++i)
    {
        auto const &element    = pElements[i];
        auto        normalized = element.normalized ? GL_TRUE : GL_FALSE;

        GL_CHECK(glEnableVertexAttribArray(i));
        GL_CHECK(glVertexAttribPointer(
          i,
          element.count,
          element.type,
          normalized,
          bl.stride(),
          (void const *)(offset)));

        offset += element.count * sizeFromGLEnum(element.type);
    }
}

U32_t sizeFromGLEnum(U32_t type)
{
    U32_t ret = 0;
    GL_UNSIGNED_BYTE;
    switch (type)
    {
    case GL_HALF_FLOAT:
        ret = 2;
        break;
    case GL_FLOAT:
        ret = 4;
        break;
    case GL_DOUBLE:
        ret = 8;
        break;
    case GL_FIXED:
        ret = 4;
        break;

    case GL_BYTE:
        ret = 1;
        break;
    case GL_UNSIGNED_BYTE:
        ret = 1;
        break;
    case GL_SHORT:
        ret = 2;
        break;
    case GL_UNSIGNED_SHORT:
        ret = 2;
        break;
    case GL_INT:
        ret = 4;
        break;
    case GL_UNSIGNED_INT:
        ret = 4;
        break;

    case GL_UNSIGNED_INT_2_10_10_10_REV:
        ret = 4;
        break;
    // requires OpenGL 4.4 or ARB_vertex_type_10f_11f_11f_rev
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
        ret = 3;
        break;
    default:
        assert(false && "invalid GL type");
    }

    return ret;
}

std::underlying_type_t<EAccess> operator&(EAccess a, EAccess b)
{
    using IntType = std::underlying_type_t<EAccess>;
    return (IntType)a & (IntType)b;
}
} // namespace cge
