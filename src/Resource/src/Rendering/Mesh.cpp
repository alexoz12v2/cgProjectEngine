#include "Rendering/Mesh.h"

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cassert>

namespace cge
{

tl::optional<Scene_s> Scene_s::open(Char8_t const *path)
{
    if (aiScene const *scene =
          aiImportFile(path, aiProcessPreset_TargetRealtime_Fast);
        scene)
    {
        MeshSpec_t const spec{ .pScene = scene };
        return tl::make_optional<Scene_s>(spec);
    }
    else { return tl::nullopt; }
}

Mesh_s Scene_s::getMesh() const
{
    Mesh_s outMesh;
    U32_t  count = m_scene->mNumMeshes;

    // TODO:
    // https://stackoverflow.com/questions/72075776/assimp-mnummeshes-is-0
    if (count != 0)
    {
        aiMesh const *mesh =
          m_scene->mMeshes[m_scene->mRootNode->mChildren[0]->mMeshes[0]];
        assert(
          mesh->HasPositions() && mesh->HasNormals() && mesh->HasFaces()
          && mesh->HasTextureCoords(0));

        aiVector3D const *vertices  = mesh->mVertices;
        aiFace const     *indices   = mesh->mFaces;
        aiVector3D const *normals   = mesh->mNormals;
        aiVector3D const *texCoords = mesh->mTextureCoords[0]; /// @warning

        outMesh.vertices.reserve(mesh->mNumVertices);
        for (U32_t i = 0; i != mesh->mNumVertices; ++i)
        {
            Vertex_t vertex;
            vertex.pos[0] = vertices[i].x;
            vertex.pos[1] = vertices[i].y;
            vertex.pos[2] = vertices[i].z;

            vertex.norm[0] = normals[i].x;
            vertex.norm[1] = normals[i].y;
            vertex.norm[2] = normals[i].z;

            vertex.texCoords[0] = texCoords[i].x;
            vertex.texCoords[1] = texCoords[i].y;
            vertex.texCoords[2] = texCoords[i].z;

            outMesh.vertices.push_back(vertex);
        }

        outMesh.indices.reserve(mesh->mNumFaces * 3);
        for (U32_t i = 0; i != mesh->mNumFaces; ++i)
        {
            assert(indices[i].mNumIndices == 3);
            Array<U32_t, 3> arr = { indices[i].mIndices[0],
                                    indices[i].mIndices[1],
                                    indices[i].mIndices[2] };
            // tried emplace_back, couldn't make it compile
            outMesh.indices.push_back(arr);
        }
    }
    // NRVO kicks in
    return outMesh;
}

Scene_s::~Scene_s() { aiReleaseImport(m_scene); }

Scene_s::Scene_s(MeshSpec_t const &spec) : m_scene(spec.pScene) {}

} // namespace cge
