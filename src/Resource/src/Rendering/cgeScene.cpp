#include "Rendering/cgeScene.h"

#include "HandleTable.h"
#include "Rendering/cgeMesh.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glad/gl.h>
#include <stb/stb_image.h>

#include <cassert>

namespace cge
{
Scene_s g_scene;

struct STBIDeleter
{
    void operator()(Byte_t *data) const { stbi_image_free(data); }
};

// TODO more
std::vector<Sid_t> loadMaterialTexture(const Char8_t *basePath, aiMaterial const *mat)
{
    static char        buffer[1024];
    std::vector<Sid_t> textures;

    for (auto type : { aiTextureType_DIFFUSE, aiTextureType_SPECULAR })
    {
        for (U32_t i = 0; i != mat->GetTextureCount(type); i++)
        {
            aiString path;
            mat->GetTexture(type, i, &path);

            // check if texture has been already loaded
            B8_t skip = false;
            auto ref  = g_handleTable.get(CGE_SID(path.C_Str()));
            if (ref.sid() != nullSid)
            {
                textures.push_back(ref.sid());
                skip = true;
                break;
            }

            // otherwise load it
            if (!skip)
            {
                I32_t texWidth      = 0;
                I32_t texHeight     = 0;
                I32_t texChannelCnt = 0;

                auto completePath = std::string(basePath);
                auto pos = completePath.find_last_of('/');
                assert(pos != std::string::npos);
                completePath = completePath.substr(0, pos) + '/';
                completePath += path.C_Str();

                // TODO not handling RGBA
                Byte_t *texData = reinterpret_cast<Byte_t *>(stbi_load(
                  completePath.c_str(),
                  &texWidth,
                  &texHeight,
                  &texChannelCnt,
                  3));

                assert(texWidth != 0);
                auto data = std::shared_ptr<Byte_t>(texData, STBIDeleter{});

                TextureData_s tex{ .data   = data,
                                   .width  = (U32_t)texWidth,
                                   .height = (U32_t)texHeight,
                                   .depth  = 1,
                                   .format = GL_UNSIGNED_BYTE,
                                   .type   = GL_RGB };
                textures.push_back(CGE_SID(path.C_Str()));
                g_handleTable.insertTexture(CGE_SID(path.C_Str()), tex);
            }
        }
    }

    return textures;
}

void processMesh(
  aiMesh const  *aMesh,
  aiScene const *aScene,
  aiNode const  *aNode,
  char const *path,
  Mesh_s        *outMesh)
{
    assert(
      aMesh->HasPositions() && aMesh->HasNormals() && aMesh->HasFaces()
      && aMesh->HasTextureCoords(0));

    // TODO: use aScene to get materials
    // allocateResources(sid, aScene, node, outScene)

    aiVector3D const *vertices  = aMesh->mVertices;
    aiFace const     *indices   = aMesh->mFaces;
    aiVector3D const *normals   = aMesh->mNormals;
    aiVector3D const *texCoords = aMesh->mTextureCoords[0]; /// @warning

    outMesh->vertices.reserve(aMesh->mNumVertices);
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

        outMesh->vertices.push_back(vertex);
    }

    outMesh->indices.reserve(aMesh->mNumFaces * 3);
    for (U32_t i = 0; i != aMesh->mNumFaces; ++i)
    {
        assert(indices[i].mNumIndices == 3);
        Array<U32_t, 3> arr = { indices[i].mIndices[0],
                                indices[i].mIndices[1],
                                indices[i].mIndices[2] };
        // tried emplace_back, couldn't make it compile
        outMesh->indices.push_back(arr);
    }

    if (aMesh->mMaterialIndex < (U32_t)-1)
    {
        aiMaterial const *material = aScene->mMaterials[aMesh->mMaterialIndex];
        outMesh->textures          = loadMaterialTexture(path, material);
    }

    // TODO set OpenGL related mesh information coming from another file?
    // outMesh.allocateResrou
}

B8_t meshNotAllocated(Sid_t sid)
{
    auto meshRef = g_handleTable.get(sid);
    return !meshRef.hasValue();
}

void Scene_s::processNode(
  Sid_t          parent,
  aiNode const  *node,
  aiScene const *aScene,
  Scene_s       *outScene)
{


    static Sid_t constexpr vertShader     = "DEFAULT_VERTEX"_sid;
    static Sid_t constexpr fragShader     = "DEFAULT_FRAG"_sid;
    static char const *const vertexSource = R"a(
#version 460 core
out gl_PerVertex
{
    vec4 gl_Position;
};

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec3 aTexCoord;

layout (location = 0) out vec3 texCoord;

layout (std140) uniform MeshUniforms {
    mat4 modelView;
    mat4 modelViewProj;
};

void main() 
{
    texCoord = aTexCoord;
    gl_Position = modelViewProj * vec4(aPos, 1.f);
}
)a";

    static char const *const fragSource = R"a(
#version 460 core
layout (location = 0) in vec3 texCoord;

uniform sampler2D sampler;

layout (location = 0) out vec4 fragColor;

void main()
{
    vec3 color = texture(sampler, texCoord.xy).xyz;
    fragColor = vec4(color, 1.f);
}
)a";

    Sid_t sid = CGE_SID(node->mName.C_Str());
    // load mesh
    assert(node->mNumMeshes <= 1 && "blender meshes should have 1 node");
    for (U32_t i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh const *aMesh = aScene->mMeshes[node->mMeshes[i]];

        // allocate all resources in handletable (materials tex) which
        // were not already allocated
        // allocateResources(sid, aScene, node, outScene);

        // nodes may refer to the same mesh
        if (meshNotAllocated(sid))
        {
            // allocate mesh in the handle table Mesh_s mesh =
            Mesh_s &mesh = g_handleTable.insertMesh(sid);

            // if the mesh is new
            processMesh(aMesh, aScene, node, m_path.c_str(), &mesh);
            mesh.allocateTexturesToGpu();

            static U32_t constexpr stagesCount = 2;
            char const *sources[stagesCount]   = { vertexSource, fragSource };
            U32_t       stages[stagesCount]    = { GL_VERTEX_SHADER,
                                                   GL_FRAGMENT_SHADER };
            mesh.shaderProgram.build({ .sid          = "DEFAULT_PROG"_sid,
                                       .pSources     = sources,
                                       .pStages      = stages,
                                       .sourcesCount = stagesCount });
            mesh.setupUniforms();
            mesh.allocateGeometryBuffersToGpu();
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

tl::optional<Scene_s> Scene_s::fromObj(Char8_t const *relativePath)
{
    Assimp::Importer importer;
    aiScene const   *scene = importer.ReadFile(
      relativePath,
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
    internalScene.m_path = relativePath;
    internalScene.processNode(rootId, scene->mRootNode, scene, &internalScene);
    return internalScene;
}

inline SceneNode_s &Scene_s::createNode(Sid_t sid, glm::mat4 const &transform)
{
    auto const &[pairIt, bWasInserted] =
      m_bnodes.try_emplace(sid, sid, transform, transform, nullptr, false);
    assert(bWasInserted && "ID collision!!");
    return pairIt->second;
}

inline B8_t Scene_s::addChild(Sid_t parent, Sid_t child)
{
    SceneNode_s *parentNode = getNodeBySid(parent);
    SceneNode_s *childNode  = getNodeBySid(child);

    if (parentNode && childNode)
    {
        childNode->parent = parentNode; // Set the parent for the child
        childNode->absoluteTransform =
          childNode->relativeTransform * parentNode->absoluteTransform;
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
