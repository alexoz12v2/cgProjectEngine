
#include "HandleTable.h"
#include "Core/Alloc.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glad/gl.h>
#include <stb/stb_image.h>

#include <algorithm>
#include <cassert>

namespace cge
{

HandleTable_s              g_handleTable;
HandleTable_s::Ref_s const nullRef = HandleTable_s::Ref_s::nullRef();

Mesh_s &HandleTable_s::insertMesh(Sid_t sid)
{
    auto [it, wasInserted] =
      meshTable.try_emplace(sid /* emplacing a default constructed object */);
    return it->second;
}

Mesh_s &HandleTable_s::insertMesh(Sid_t sid, Mesh_s mesh)
{
    auto [it, wasInserted] = meshTable.try_emplace(sid, mesh);
    return it->second;
}

TextureData_s &
  HandleTable_s::insertTexture(Sid_t sid, TextureData_s const &texture)
{
    auto [it, wasInserted] = textureTable.try_emplace(sid, texture);
    return it->second;
}

B8_t HandleTable_s::remove(Sid_t sid)
{
    auto it0 = meshTable.find(sid);
    auto it1 = lightTable.find(sid);
    auto it2 = textureTable.find(sid);
    if (it0 != meshTable.cend())
    {
        meshTable.erase(it0);
        return true;
    }
    else if (it1 != lightTable.cend())
    {
        lightTable.erase(it1);
        return true;
    }
    else if (it2 != textureTable.cend())
    {
        textureTable.erase(it2);
        return true;
    }
    else { return false; }
}

HandleTable_s::Ref_s HandleTable_s::get(Sid_t sid)
{
    Ref_s ref;
    auto  it0 = meshTable.find(sid);
    auto  it1 = lightTable.find(sid);
    auto  it2 = textureTable.find(sid);
    if (it0 != meshTable.cend())
    {
        ref.m_sid  = sid;
        ref.m_ptr  = &it0->second;
        ref.m_type = EResourceType_t::eMesh;
        return ref;
    }
    else if (it1 != lightTable.cend())
    {
        ref.m_sid  = sid;
        ref.m_ptr  = &it1->second;
        ref.m_type = EResourceType_t::eLight;
        return ref;
    }
    else if (it2 != textureTable.cend())
    {
        ref.m_sid  = sid;
        ref.m_ptr  = &it2->second;
        ref.m_type = EResourceType_t::eTexture;
        return ref;
    }
    return ref;
}

Mesh_s &HandleTable_s::Ref_s::asMesh() { return *(Mesh_s *)m_ptr; }

Light_t &HandleTable_s::Ref_s::asLight() { return *(Light_t *)m_ptr; }

TextureData_s &HandleTable_s::Ref_s::asTexture()
{
    return *(TextureData_s *)m_ptr;
}

Mesh_s const &HandleTable_s::Ref_s::asMesh() const
{
    return *(Mesh_s const *)m_ptr;
}

Light_t const &HandleTable_s::Ref_s::asLight() const
{
    return *(Light_t const *)m_ptr;
}

TextureData_s const &HandleTable_s::Ref_s::asTexture() const
{
    return *(TextureData_s const *)m_ptr;
}

B8_t HandleTable_s::Ref_s::hasValue() const
{
    return m_ptr != nullptr && m_sid != nullSid;
}

[[nodiscard]] Sid_t HandleTable_s::Ref_s::sid() const { return m_sid; }

[[nodiscard]] HandleTable_s::Ref_s const &HandleTable_s::Ref_s::nullRef()
{
    static const Ref_s ref;
    return ref;
}

} // namespace cge
