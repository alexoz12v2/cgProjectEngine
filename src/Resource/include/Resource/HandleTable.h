#pragma once

#include "Core/Containers.h"
#include "Core/Module.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Resource/Rendering/cgeLight.h"
#include "Resource/Rendering/cgeMesh.h"

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

class HandleTable_s
{
  public:
    class Ref_s
    {
        friend HandleTable_s;

      public:
        Mesh_s        &asMesh();
        TextureData_s &asTexture();

        Mesh_s const        &asMesh() const;
        TextureData_s const &asTexture() const;

        [[nodiscard]] B8_t hasValue() const;

        [[nodiscard]] Sid_t sid() const;

        [[nodiscard]] static Ref_s const &nullRef();

      private:
        constexpr Ref_s() = default;

        Sid_t           m_sid  = nullSid;
        EResourceType_t m_type = EResourceType_t::eInvalid;
        void           *m_ptr  = nullptr;
    };

  public:
    Mesh_s              &insertMesh(Sid_t sid, Mesh_s const &mesh);
    Mesh_s              &insertMesh(Sid_t sid);
    TextureData_s       &insertTexture(Sid_t sid, TextureData_s const &texture);
    B8_t                 remove(Sid_t sid);
    HandleTable_s::Ref_s get(Sid_t sid);
    Mesh_s              &getMesh(Sid_t sid);
    TextureData_s       &getTexture(Sid_t sid);

    void loadFromObj(Char8_t const *path);

  private:
    void loadTextures(Char8_t const *path, void const *material, Mesh_s &mesh);

  private:
    // using map for iterator stability
    std::pmr::map<Sid_t, Mesh_s>        m_meshTable{ getMemoryPool() };
    std::pmr::map<Sid_t, TextureData_s> m_textureTable{ getMemoryPool() };
};

extern HandleTable_s::Ref_s const nullRef;
extern HandleTable_s              g_handleTable;
} // namespace cge
