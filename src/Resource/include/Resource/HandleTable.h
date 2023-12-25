#pragma once

#include "Core/Containers.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Resource/Rendering/cgeLight.h"
#include "Resource/Rendering/cgeMesh.h"

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
    eLight,
    eByteArray
};

class HandleTable_s
{
  private:
    struct Value_t
    {
        explicit Value_t(Mesh_s const &m) : type(EResourceType_t::eMesh), res(m)
        {
        }
        explicit Value_t(Light_t l) : type(EResourceType_t::eLight), res(l) {}

        Value_t(const Value_t &other) : type(other.type), res()
        {
            using enum EResourceType_t;
            switch (type)
            {
            case eMesh:
                new (&res.mesh) Mesh_s(other.res.mesh);
                break;
            case eLight:
                new (&res.light) Light_t(other.res.light);
                break;
            }
        }
        Value_t(Value_t &&other)                     = default;
        Value_t &operator=(Value_t const &other)     = default;
        Value_t &operator=(Value_t &&other) noexcept = default;

        ~Value_t() noexcept
        {
            using enum EResourceType_t;
            switch (type)
            {
            case eMesh:
                res.mesh.~Mesh_s();
                break;
            case eLight:
                res.light.~Light_t();
                break;
            }
        }
        EResourceType_t type;
        union U
        {
            U(){};
            U(Mesh_s const &m) : mesh(m) {}
            U(Light_t l) : light(l) {}
            ~U() {}
            Mesh_s  mesh;
            Light_t light;
        };
        U res;
    };

  public:
    // using map for iterator stability
    using HandleMap_s = std::pmr::map<Sid_t, std::pair<U32_t, Value_t>>;

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

        Mesh_s  &getAsMesh() { return m_ptr->second.second.res.mesh; }
        Light_t &getAsLight() { return m_ptr->second.second.res.light; }

        B8_t hasValue()
        {
            return m_pTable != nullptr && m_ptr != m_pTable->m_map.cend();
        }

        Sid_t sid() const { return m_sid; }

      private:
        Ref_s() = default;
        Sid_t                 m_sid;
        HandleMap_s::iterator m_ptr;
        HandleTable_s        *m_pTable;
    };

    void                 insertMesh(Sid_t sid, Mesh_s const &mesh);
    B8_t                 remove(Sid_t sid);
    HandleTable_s::Ref_s get(Sid_t sid);

  private:
    B8_t remove(HandleMap_s::iterator ptr);
    // id -> { reference count, resource }
    HandleMap_s m_map;
};

extern HandleTable_s g_handleTable;
} // namespace cge
