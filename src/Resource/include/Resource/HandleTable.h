#pragma once

#include "Core/Containers.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Resource/Rendering/Mesh.h"

#include <atomic>
#include <map>
#include <tl/optional.hpp>

namespace cge
{

enum class EResourceType_t
{
    eInvalid = 0,
    eMesh,
    eTexture,
    eByteArray
};

struct Resource_s
{
    union ResourceValue_t
    {
        void   *p;
        Mesh_s *pMesh;
        Byte_t *pByteArray;
    };

    // value is supposed to be dynamically allocated
    Resource_s(EResourceType_t type, void *value)
      : type(type), value{ .p = value }
    {
    }

    EResourceType_t type;
    ResourceValue_t value;
};

class HandleTable_s
{
    // using map for iterator stability
    using HandleMap_s = std::map<Sid_t, std::pair<U32_t, Resource_s>>;

  public:
    class Ref_s
    {
        friend HandleTable_s;

      public:
        Ref_s(Ref_s const &other)
          : m_sid(other.m_sid), m_ptr(other.m_ptr), m_pTable(other.m_pTable)
        {
            if (hasValue()) { ++m_ptr->second.first; }
        }

        Ref_s(Ref_s &&other) noexcept
          : m_sid(std::exchange(other.m_sid, nullSid)),
            m_ptr(std::exchange(other.m_ptr, other.m_pTable->m_map.end())),
            m_pTable(std::exchange(other.m_pTable, nullptr))
        {
        }

        Ref_s &operator=(Ref_s const &other)
        {
            m_sid    = other.m_sid;
            m_ptr    = other.m_ptr;
            m_pTable = other.m_pTable;
            ++m_ptr->second.first;
            return *this;
        }

        Ref_s &operator=(Ref_s &&other) noexcept
        {
            m_sid    = std::exchange(other.m_sid, nullSid);
            m_ptr    = std::exchange(other.m_ptr, other.m_pTable->m_map.end());
            m_pTable = std::exchange(other.m_pTable, nullptr);
            return *this;
        }

        template<typename T> T &getAs()
        {
            return *(T *)m_ptr->second.second.value.p;
        }

        B8_t hasValue()
        {
            return m_pTable != nullptr && m_ptr != m_pTable->m_map.cend();
        }

        ~Ref_s() noexcept { m_pTable->remove(m_ptr); }

        Sid_t sid() const { return m_sid; }

      private:
        Ref_s() = default;
        Sid_t                 m_sid;
        HandleMap_s::iterator m_ptr;
        HandleTable_s        *m_pTable;
    };
    void insert(Sid_t sid, Resource_s const &ref);

    B8_t remove(Sid_t sid);

    HandleTable_s::Ref_s get(Sid_t sid);

  private:
    B8_t remove(HandleMap_s::iterator ptr);
    // id -> { reference count, resource }
    HandleMap_s m_map;
};

extern HandleTable_s g_handleTable;
} // namespace cge
