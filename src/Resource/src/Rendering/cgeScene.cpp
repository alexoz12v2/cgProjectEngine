#include "Rendering/cgeScene.h"

#include "HandleTable.h"
#include "Rendering/cgeMesh.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cassert>

namespace cge
{

Scene_s g_scene;
Mesh_s  processMesh(aiMesh *aMesh, aiScene const *aScene)
{
    Mesh_s outMesh;
    assert(
      aMesh->HasPositions() && aMesh->HasNormals() && aMesh->HasFaces()
      && aMesh->HasTextureCoords(0));

    // TODO: use aScene to get materials
    // allocateResources(sid, aScene, node, outScene)

    aiVector3D const *vertices  = aMesh->mVertices;
    aiFace const     *indices   = aMesh->mFaces;
    aiVector3D const *normals   = aMesh->mNormals;
    aiVector3D const *texCoords = aMesh->mTextureCoords[0]; /// @warning

    outMesh.vertices.reserve(aMesh->mNumVertices);
    for (U32_t i = 0; i != aMesh->mNumVertices; ++i)
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

    outMesh.indices.reserve(aMesh->mNumFaces * 3);
    for (U32_t i = 0; i != aMesh->mNumFaces; ++i)
    {
        assert(indices[i].mNumIndices == 3);
        Array<U32_t, 3> arr = { indices[i].mIndices[0],
                                indices[i].mIndices[1],
                                indices[i].mIndices[2] };
        // tried emplace_back, couldn't make it compile
        outMesh.indices.push_back(arr);
    }

    return outMesh;
}

B8_t meshNotAllocated(Sid_t sid)
{
    auto meshRef = g_handleTable.get(sid); //
    return !meshRef.hasValue();
}

void processNode(
  Sid_t          parent,
  aiNode        *node,
  aiScene const *aScene,
  Scene_s       *outScene)
{
    Sid_t sid = CGE_SID(node->mName.C_Str());
    // load mesh
    assert(node->mNumMeshes <= 1 && "blender meshes should have 1 node");
    for (U32_t i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh *aMesh = aScene->mMeshes[node->mMeshes[i]];

        // allocate all resources in handletable (materials tex) which
        // were not already allocated
        // allocateResources(sid, aScene, node, outScene);

        // nodes may refer to the same mesh
        if (meshNotAllocated(sid))
        {
            // if the mesh is new
            Mesh_s mesh = processMesh(aMesh, aScene);

            // allocate mesh in the handle table
            g_handleTable.insertMesh(sid, mesh);
        }

        // create new node in scene
        outScene->createNode(sid);

        // set parent
        outScene->addChild(parent, sid);

        // recurse (works only if there is 1 mesh per node)
        for (U32_t i = 0; i < node->mNumChildren; ++i)
        {
            processNode(sid, node->mChildren[i], aScene, outScene);
        }
    }
    if (node->mNumMeshes == 0)
    {
        for (U32_t i = 0; i < node->mNumChildren; ++i)
        {
            processNode(sid, node->mChildren[i], aScene, outScene);
        }
    }
}

tl::optional<Scene_s> Scene_s::fromObj(Char8_t const *path)
{
    Assimp::Importer importer;
    aiScene const   *scene = importer.ReadFile(
      path,
      aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
        | aiProcess_ForceGenNormals);
    if (
      !scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        return tl::nullopt;
    }
    Scene_s internalScene;
    Sid_t   rootId = CGE_SID(scene->mRootNode->mName.C_Str());
    internalScene.m_names.push_back(rootId);
    processNode(rootId, scene->mRootNode, scene, &internalScene);
    return internalScene;
}

inline SceneNode_s &Scene_s::createNode(Sid_t sid, glm::mat4 const &transform)
{
    auto pair = m_bnodes.try_emplace(sid, sid, transform, nullptr);
    assert(pair.second && "ID collision!!");
    return pair.first->second;
}

inline B8_t Scene_s::addChild(Sid_t parent, Sid_t child)
{
    SceneNode_s *parentNode = getNodeBySid(parent);
    SceneNode_s *childNode  = getNodeBySid(child);

    if (parentNode && childNode)
    {
        childNode->parent = parentNode; // Set the parent for the child
        parentNode->children.push_front(childNode);
        return true;
    }
    else { return false; }
}

inline B8_t Scene_s::removeChild(Sid_t parent, Sid_t child)
{
    if (SceneNode_s *parentNode = getNodeBySid(parent))
    {
        auto elemntsRemovedCnt = parentNode->children.remove_if(
          [child](SceneNode_s const *childNode)
          { return childNode->sid == child && childNode->children.empty(); });

        return elemntsRemovedCnt != 0;
    }
    return false;
}

inline B8_t Scene_s::removeNode(Sid_t sid)
{
    auto it = m_bnodes.find(sid);

    if (it != m_bnodes.end() && it->second.children.empty())
    {
        // Remove the node if it has no children
        if (it->second.parent)
        {
            // Remove the node from its parent's children list
            // TODO remove sid
            it->second.parent->children.remove_if(
              [sid](SceneNode_s const *childNode)
              { return childNode->sid == sid; });
        }

        m_bnodes.erase(it);
        return true;
    }
    else { return false; }
}
} // namespace cge
