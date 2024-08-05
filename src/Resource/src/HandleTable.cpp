#include "HandleTable.h"

#include "Core/Alloc.h"
#include "HandleTable.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glad/gl.h>
#include <stb/stb_image.h>

#include <algorithm>
#include <cassert>
#include <queue>
#include <string>

namespace cge
{

struct STBIDeleter
{
    void operator()(Byte_t *data) const { stbi_image_free(data); }
};

static char const *const vertexSource = R"a(
#version 460 core
out gl_PerVertex
{
    vec4 gl_Position;
};

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec4 aColor; // <<
layout (location = 4) in float aShininess; // <<

layout (location = 0) out vec3 texCoord;
layout (location = 1) out vec4 vColor;
layout (location = 2) out vec3 vNormal;
layout (location = 3) out float vShininess;
layout (location = 4) out vec3 vPosition;

layout (std140) uniform MeshUniforms {
    mat4 modelView;
    mat4 modelViewProj;
};

void main() 
{
    texCoord = aTexCoord;
    vColor = aColor;
    vNormal = aNorm;
    vShininess = aShininess;
    vPosition = aPos;
    gl_Position = modelViewProj * vec4(aPos, 1.f);
}
)a";

static char const *const fragSource = R"a(
#version 460 core

struct LightProperties {
    bool isEnabled;
    bool isLocal;
    bool isSpot;
    vec3 ambient;
    vec3 color;
    vec3 position;
    vec3 halfVector;
    vec3 coneDirection;
    float spotCosCutoff;
    float spotExponent;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
};

// only light of the scene
uniform LightProperties light;

// albedo
uniform sampler2D albedoSampler;
uniform bool hasAlbedoTexture;

// normal
uniform sampler2D normalSampler;
uniform bool hasNormalSampler;

// shininess
uniform sampler2D shininessSampler;
uniform bool hasShininessSampler;

// constants
uniform vec3 eyeDirection;

// per vertex fallbacks if textures are absent (+position)
layout (location = 0) in vec3 texCoord;
layout (location = 1) in vec4 vColor;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in float vShininess;
layout (location = 4) in vec3 vPosition;

layout (location = 0) out vec4 fragColor;

void main() {
    // compute material color, normal and shininess
    vec4 mColor;
    vec3 mNormal;
    float mShininess;

    if (hasAlbedoTexture) {
        mColor = texture(albedoSampler, texCoord.xy);
    } else {
        mColor = vColor;
    }

    if (hasNormalSampler) {
        mNormal = texture(normalSampler, texCoord.xy).xyz;
    } else {
        mNormal = vNormal;
    }

    if (hasShininessSampler) {
        mShininess = texture(shininessSampler, texCoord.xy).x;
    } else {
        mShininess = vShininess;
    }

    // accumulate scattered light and reflected light for each light
    vec3 scatteredLight = vec3(0.f);
    vec3 reflectedLight = vec3(0.f);

    if (light.isEnabled) {
        vec3 halfVector;
        vec3 lightDirection = light.position;
        float attenuation = 1.f;
        
        // for local lights, compute per-fragment direction, halfVector and attenuation
        if (light.isLocal) {
            lightDirection = lightDirection - vPosition;
            float lightDistance = length(lightDirection);
            lightDirection = lightDirection / lightDistance;

            attenuation = 1.f / (
                light.constantAttenuation +
                light.linearAttenuation * lightDistance + 
                light.quadraticAttenuation * lightDistance * lightDistance);

            if (light.isSpot) {
                float spotCos = dot(lightDirection, -light.coneDirection);
                if (spotCos < light.spotCosCutoff) {
                    attenuation = 0.f;
                } else {
                    attenuation *= pow(spotCos, light.spotExponent);
                }
            }

            halfVector = normalize(lightDirection + eyeDirection);
        } else {
            halfVector = light.halfVector;
        }

        // compute diffuse and specular contributions of the current light
        float diffuse = max(0.f, dot(mNormal, lightDirection));
        float specular = max(0.f, dot(mNormal, halfVector));

        if (diffuse == 0.f) {
            specular = 0.f;
        } else {
            specular = pow(specular, vShininess);
        }

        scatteredLight += (light.ambient + light.color * diffuse) * attenuation;
        reflectedLight += light.color * specular * attenuation;
    }

    vec3 rgb = min(mColor.rgb * scatteredLight + reflectedLight, vec3(1.f));
    fragColor = vec4(rgb, mColor.a);
}
)a";


HandleTable_s              g_handleTable;
HandleTable_s::Ref_s const nullRef = HandleTable_s::Ref_s::nullRef();

Mesh_s &HandleTable_s::insertMesh(Sid_t sid)
{
    auto [it, wasInserted] =
      m_meshTable.try_emplace(sid /* emplacing a default constructed object */);
    return it->second;
}

Mesh_s &HandleTable_s::insertMesh(Sid_t sid, Mesh_s const &mesh)
{
    auto [it, wasInserted] = m_meshTable.try_emplace(sid, mesh);
    return it->second;
}

TextureData_s &
  HandleTable_s::insertTexture(Sid_t sid, TextureData_s const &texture)
{
    auto [it, wasInserted] = m_textureTable.try_emplace(sid, texture);
    return it->second;
}

Light_t &HandleTable_s::insertLight(Sid_t sid, Light_t const &light)
{
    auto [it, wasInserted] = m_lightTable.try_emplace(sid, light);
    return it->second;
}

B8_t HandleTable_s::remove(Sid_t sid)
{
    auto it0 = m_meshTable.find(sid);
    auto it1 = m_lightTable.find(sid);
    auto it2 = m_textureTable.find(sid);
    if (it0 != m_meshTable.cend())
    {
        m_meshTable.erase(it0);
        return true;
    }
    else if (it1 != m_lightTable.cend())
    {
        m_lightTable.erase(it1);
        return true;
    }
    else if (it2 != m_textureTable.cend())
    {
        m_textureTable.erase(it2);
        return true;
    }
    else { return false; }
}

HandleTable_s::Ref_s HandleTable_s::get(Sid_t sid)
{
    Ref_s ref;
    auto  it0 = m_meshTable.find(sid);
    auto  it1 = m_lightTable.find(sid);
    auto  it2 = m_textureTable.find(sid);
    if (it0 != m_meshTable.cend())
    {
        ref.m_sid  = sid;
        ref.m_ptr  = &it0->second;
        ref.m_type = EResourceType_t::eMesh;
        return ref;
    }
    else if (it1 != m_lightTable.cend())
    {
        ref.m_sid  = sid;
        ref.m_ptr  = &it1->second;
        ref.m_type = EResourceType_t::eLight;
        return ref;
    }
    else if (it2 != m_textureTable.cend())
    {
        ref.m_sid  = sid;
        ref.m_ptr  = &it2->second;
        ref.m_type = EResourceType_t::eTexture;
        return ref;
    }
    return ref;
}

std::pmr::map<Sid_t, Light_t>::const_iterator HandleTable_s::lightsBegin() const
{ //
    return m_lightTable.cbegin();
}

std::pmr::map<Sid_t, Light_t>::const_iterator HandleTable_s::lightsEnd() const
{ //
    return m_lightTable.cend();
}

static void pushInQueue(std::pmr::vector<aiNode *> &queue, aiNode *node)
{
    queue.push_back(node);
    for (U32_t i = 0; i < node->mNumChildren; ++i)
    { //
        pushInQueue(queue, node->mChildren[i]);
    }
}

void HandleTable_s::loadFromObj(Char8_t const *path)
{ //
    Assimp::Importer importer;
    aiScene const   *scene = importer.ReadFile(
      path,
      aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
        | aiProcess_ForceGenNormals);
    if (
      !scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        printf("[HandleTable] couldn't load scene at %s\n", path);
        return;
    }

    static std::pmr::vector<aiNode *> queue{ getMemoryPool() };
    queue.clear();
    queue.reserve(64);
    pushInQueue(queue, scene->mRootNode);

    for (auto const *node : queue)
    {
        Sid_t sid = CGE_SID(node->mName.C_Str());
        assert(
          node->mNumMeshes <= 1
          && "[HandleTable] blender meshes should have 1 node");

        // if the mesh is not allocated
        if (!get(sid).hasValue() && node->mNumMeshes > 0)
        { // then allocate the mesh and insert all the relevant attributes
            Mesh_s       &mesh  = insertMesh(sid);
            aiMesh const *aMesh = scene->mMeshes[node->mMeshes[0]];
            assert(
              aMesh->HasPositions() && aMesh->HasNormals() && aMesh->HasFaces()
              && aMesh->HasTextureCoords(0));

            aiVector3D const       *vertices  = aMesh->mVertices;
            aiFace const           *indices   = aMesh->mFaces;
            aiVector3D const       *normals   = aMesh->mNormals;
            aiVector3D const       *texCoords = aMesh->mTextureCoords[0];
            aiColor4D const *const *colors    = aMesh->mColors;

            mesh.vertices.reserve(aMesh->mNumVertices);
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

                vertex.color[0] =
                  aMesh->HasVertexColors(0) ? colors[0][i].r : 0.4f;
                vertex.color[1] =
                  aMesh->HasVertexColors(0) ? colors[0][i].g : 0.4f;
                vertex.color[2] =
                  aMesh->HasVertexColors(0) ? colors[0][i].b : 0.4f;
                vertex.color[3] =
                  aMesh->HasVertexColors(0) ? colors[0][i].a : 1.f;

                if (aMesh->mMaterialIndex < (U32_t)-1)
                {
                    scene->mMaterials[aMesh->mMaterialIndex]->Get(
                      AI_MATKEY_SHININESS, vertex.shininess);
                }
                else
                { //
                    vertex.shininess = 1.f;
                }

                mesh.vertices.push_back(vertex);
            }

            mesh.indices.reserve(aMesh->mNumFaces * 3);
            for (U32_t i = 0; i != aMesh->mNumFaces; ++i)
            {
                assert(indices[i].mNumIndices == 3);
                Array<U32_t, 3> arr = { indices[i].mIndices[0],
                                        indices[i].mIndices[1],
                                        indices[i].mIndices[2] };
                mesh.indices.push_back(arr);
            }

            if (aMesh->mMaterialIndex < (U32_t)-1)
            {
                aiMaterial const *material =
                  scene->mMaterials[aMesh->mMaterialIndex];
                loadTextures(path, material, mesh);
            }

            mesh.box = computeAABB(mesh);

            // finalize mesh loading
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
    }
}

void HandleTable_s::loadTextures(
  Char8_t const *basePath,
  void const    *material,
  Mesh_s        &mesh)
{
    auto       *aMaterial = reinterpret_cast<aiMaterial const *>(material);
    static char buffer[1024];
    static aiTextureType constexpr types[]{ aiTextureType_DIFFUSE,
                                            aiTextureType_NORMALS,
                                            aiTextureType_SPECULAR };
    U32_t num = 0;
    for (U32_t j = 0; j != 3; ++j)
    {
        auto const type = types[j];
        for (U32_t i = 0; i != aMaterial->GetTextureCount(type); i++)
        {
            aiString path;
            aMaterial->GetTexture(type, i, &path);

            // check if texture has been already loaded
            auto ref = g_handleTable.get(CGE_SID(path.C_Str()));
            if (ref.sid() != nullSid)
            {
                mesh.textures.arr[j] = ref.sid();
                break;
            }

            // otherwise load it
            I32_t texWidth      = 0;
            I32_t texHeight     = 0;
            I32_t texChannelCnt = 0;

            std::string completePath{ basePath };
            auto        pos = completePath.find_last_of('/');
            assert(pos != std::string::npos);
            completePath = completePath.substr(0, pos) + '/';
            completePath += path.C_Str();

            // TODO not handling RGBA
            Byte_t *texData = reinterpret_cast<Byte_t *>(stbi_load(
              completePath.c_str(), &texWidth, &texHeight, &texChannelCnt, 3));

            assert(texWidth != 0);
            auto data = std::shared_ptr<Byte_t>(texData, STBIDeleter{});

            TextureData_s tex{ .data   = data,
                               .width  = (U32_t)texWidth,
                               .height = (U32_t)texHeight,
                               .depth  = 1,
                               .format = GL_UNSIGNED_BYTE,
                               .type   = GL_RGB };
            mesh.textures.arr[j] = CGE_SID(path.C_Str());
            g_handleTable.insertTexture(CGE_SID(path.C_Str()), tex);
            switch (type)
            {
            case aiTextureType_DIFFUSE:
                mesh.hasDiffuse = true;
                break;
            case aiTextureType_NORMALS:
                mesh.hasNormal = true;
                break;
            case aiTextureType_SPECULAR:
                mesh.hasSpecular = true;
                break;
            }

            // 1 texture per type
            break;
        }
        num++;
    }

    mesh.numTextures = num;
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
