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
    struct Empty_t
    {
    };
    struct Value_t
    {
        explicit Value_t(Mesh_s const &m) : type(EResourceType_t::eMesh), res(m)
        {
        }
        explicit Value_t(Light_t l) : type(EResourceType_t::eLight), res(l) {}
        explicit Value_t(TextureData_s const &t)
          : type(EResourceType_t::eTexture), res(t)
        {
        }
        explicit Value_t(Empty_t) : type(EResourceType_t::eInvalid) {}

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
            case eTexture:
                new (&res.texture) TextureData_s(other.res.texture);
                break;
            case eInvalid:
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
            case eTexture:
                res.texture.~TextureData_s();
                break;
            case eInvalid:
                break;
            }
        }
        EResourceType_t type;
        union U
        {
            U(){};
            U(Empty_t){};
            U(Mesh_s const &m) : mesh(m) {}
            U(Light_t l) : light(l) {}
            // a unique_ptr is not copyable
            U(TextureData_s const &t) : texture(t) {}
            ~U() {}
            Mesh_s        mesh;
            Light_t       light;
            TextureData_s texture;
        };
        U res;
    };

  public:
    class Ref_s
    {
        friend HandleTable_s;

      public:
        Mesh_s        &getAsMesh() { return *(Mesh_s *)m_ptr; }
        Light_t       &getAsLight() { return *(Light_t *)m_ptr; }
        TextureData_s &getAsTexture() { return *(TextureData_s *)m_ptr; }

        B8_t hasValue() const { return m_ptr; }

        Sid_t sid() const { return m_sid; }

      private:
        Ref_s() = default;

        Sid_t           m_sid  = nullSid;
        EResourceType_t m_type = EResourceType_t::eInvalid;
        void           *m_ptr  = nullptr;
    };
    Mesh_s              &insertMesh(Sid_t sid, Mesh_s mesh);
    Mesh_s              &insertMesh(Sid_t sid);
    TextureData_s       &insertTexture(Sid_t sid, TextureData_s const &texture);
    B8_t                 remove(Sid_t sid);
    HandleTable_s::Ref_s get(Sid_t sid);

    // using map for iterator stability
    std::map<Sid_t, Mesh_s>        meshTable;
    std::map<Sid_t, Light_t>       lightTable;
    std::map<Sid_t, TextureData_s> textureTable;
};

extern HandleTable_s g_handleTable;
} // namespace cge
