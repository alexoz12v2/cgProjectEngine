#pragma once

#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Resource/HandleTable.h"

#include <tl/optional.hpp>
#include <type_traits>
#include <utility>

extern void onRefDestroy(Byte_t *CGE_restrict hTabFinger);
extern bool onRefWriteAccess(
  Byte_t *CGE_restrict hTabFinger,
  U32_t *CGE_restrict  outLock);

extern void onRefCopy(Byte_t *CGE_restrict hTabFinger);
extern void
  onRefRelease(Byte_t *CGE_restrict hTabFinger, U32_t *CGE_restrict outLock);

struct DefaultRefType_t
{
};

struct EmptyRef_t
{
};

template<
  typename T   = DefaultRefType_t,
  auto copy    = onRefCopy,
  auto write   = onRefWriteAccess,
  auto release = onRefRelease,
  auto func    = onRefDestroy>
struct Ref
{
    friend class HandleTable_t;
    constexpr Ref()
    {
        m_ptr        = nullptr;
        m_hTabFinger = nullptr;
        sid          = s_nullSid;
    }

    Ref(Ref const &other)
      : sid(other.sid), m_ptr(other.m_ptr), m_hTabFinger(other.m_hTabFinger)
    {
        if (!null()) { copy(m_hTabFinger); }
    }

    Ref(Ref &&other)
      : sid(std::exchange(other.sid, 0)),
        m_ptr(std::exchange(other.m_ptr, nullptr)),
        m_hTabFinger(std::exchange(other.m_hTabFinger, nullptr))
    {
    }

    ~Ref()
    {
        if (m_ptr)
        {
            // call decrement on HandleTable
            func(m_hTabFinger);
        }
    }

    Ref &operator=(Ref const &other)
    {
        sid          = other.sid;
        m_ptr        = other.m_ptr;
        m_hTabFinger = other.m_hTabFinger;
        if (!null()) { copy(m_hTabFinger); }
        return *this;
    }

    Ref &operator=(Ref &&other)
    {
        sid          = std::exchange(other.sid, 0);
        m_ptr        = std::exchange(other.m_ptr, nullptr);
        m_hTabFinger = std::exchange(other.m_hTabFinger, nullptr);
        return *this;
    }

    Ref<DefaultRefType_t> toRaw()
    {
        Ref<DefaultRefType_t> rawRef;
        rawRef.sid          = sid;
        rawRef.m_ptr        = m_ptr;
        rawRef.m_hTabFinger = m_hTabFinger;
        rawRef.m_lock       = m_lock;
        return rawRef;
    }

    bool operator==(EmptyRef_t) { return m_ptr; }

    template<std::convertible_to<T> U> bool operator==(Ref<U> other)
    {
        assert(m_hTabFinger == other.m_hTabFinger && sid.id == other.sid.id);
        return other.m_ptr == m_ptr;
    }

    T const         *readAccess() const { return m_ptr; }
    tl::optional<T> *writeAccess()
    {
        return write(m_hTabFinger, &m_lock) ? m_ptr : tl::nullopt;
    }

    template<typename F> auto writeAccess(F &&func) -> bool
    {
        if (write(m_hTabFinger, &m_lock))
        {
            func(m_ptr);
            release(m_hTabFinger);
            return true;
        }
        else { return false; }
    }

    bool null() const { return !(m_hTabFinger && m_ptr); }

  public:
    Sid_t sid;

  private:
    Byte_t *m_hTabFinger;
    T      *m_ptr;
    U32_t   m_lock;
};

// TODO EmptyRef Type and equality with ref
static EmptyRef_t constexpr s_emptyRef;
