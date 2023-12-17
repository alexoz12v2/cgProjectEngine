#pragma once

#include "Core/StringUtils.h"
#include "Core/Type.h"

#include <tl/optional.hpp>
#include <type_traits>
#include <utility>
namespace cge
{


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

template<typename T> class Ref_s
{
  public:
    using Self = Ref_s<T>;
    friend class HandleTable_t;

    Ref_s() = default;
    Ref_s(Self const &other)
      : m_sid(other.m_sid), m_ptr(other.m_ptr), m_hTabFinger(other.m_hTabFinger)
    {
        if (!null()) { onRefCopy(m_hTabFinger); }
    }

    Ref_s(Self &&other) noexcept = default;

    ~Ref_s()
    {
        if (m_ptr)
        {
            // call decrement on HandleTable
            onRefRelease(m_hTabFinger, &m_lock);
            onRefDestroy(m_hTabFinger);
        }
    }

    Self &operator=(Self const &other)
    {
        m_sid        = other.m_sid;
        m_ptr        = other.m_ptr;
        m_hTabFinger = other.m_hTabFinger;
        if (!null()) { onRefCopy(m_hTabFinger); }
        return *this;
    }

    Self &operator=(Self &&other) noexcept = default;

    bool operator==(EmptyRef_t) { return m_ptr; }

    template<std::convertible_to<T> U> bool operator==(Ref_s<U> other)
    {
        assert(
          m_hTabFinger == other.m_hTabFinger && m_sid.id == other.m_sid.id);
        return other.m_ptr == m_ptr;
    }

    T const *readAccess() const { return m_ptr; }

    /// @note I don't know if we need this
    // tl::optional<T *> writeAccess()
    //{
    //     return onRefWriteAccess(m_hTabFinger, &m_lock)
    //              ? tl::optional<T *>{ m_ptr }
    //              : tl::nullopt;
    // }

    template<typename F> auto writeAccess(F &&func) -> bool
    {
        if (onRefWriteAccess(m_hTabFinger, &m_lock))
        {
            std::forward<F>(m_ptr);
            onRefRelease(m_hTabFinger, &m_lock);
            return true;
        }
        else { return false; }
    }

    bool null() const { return !(m_hTabFinger && m_ptr); }

    Sid_t sid() const { return m_sid; }

  private:
    Sid_t   m_sid        = nullSid;
    Byte_t *m_hTabFinger = nullptr;
    T      *m_ptr        = nullptr;
    U32_t   m_lock       = 0;

    Ref_s(
      const Sid_t &m_sid,
      Byte_t      *m_hTabFinger,
      T           *m_ptr,
      const U32_t &m_lock)
      : m_sid(m_sid), m_hTabFinger(m_hTabFinger), m_ptr(m_ptr), m_lock(m_lock)
    {
    }
};

template class Ref_s<Sid_t>;

static EmptyRef_t constexpr s_emptyRef;
} // namespace cge
