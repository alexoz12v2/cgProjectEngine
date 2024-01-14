
#include "HandleTable.h"
#include "Core/Alloc.h"

#include <cassert>

namespace cge
{

HandleTable_s g_handleTable;
Mesh_s       &HandleTable_s::insertMesh(Sid_t sid)
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
    auto it0 = meshTable.find(sid);
    auto it1 = lightTable.find(sid);
    auto it2 = textureTable.find(sid);
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
} // namespace cge
