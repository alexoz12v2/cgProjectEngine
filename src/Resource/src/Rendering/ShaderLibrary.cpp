//
// Created by alessio on 07/01/24.
//
#include "Rendering/ShaderLibrary.h"

#include <cstring>
#include <fstream>
#include <glad/gl.h>
#include <sstream>

namespace cge
{
tl::optional<Shader_s *> cge::ShaderLibrary_s::open(const char *path)
{
    // find extension
    char const *extension = path + strlen(path) - 4;

    auto  extSid  = CGE_SID(extension);
    auto  pathSid = CGE_SID(path);
    U32_t stage   = 0;
    switch (extSid.id)
    {
    case "vert"_sid.id:
        stage = GL_VERTEX_SHADER;
        break;
    case "frag"_sid.id:
        stage = GL_FRAGMENT_SHADER;
        break;
    case "comp"_sid.id:
        stage = GL_COMPUTE_SHADER;
        break;
    case "geom"_sid.id:
        stage = GL_GEOMETRY_SHADER;
        break;
    case "tesc"_sid.id:
        stage = GL_TESS_CONTROL_SHADER;
        break;
    case "tese"_sid.id:
        stage = GL_TESS_EVALUATION_SHADER;
        break;
    default:
        return tl::nullopt;
    }

    std::ifstream file{ path };
    if (file.fail()) { return tl::nullopt; }

    std::stringstream stream;
    stream << file.rdbuf();
    std::string source    = stream.str();
    const char *sourcePtr = source.c_str();

    U32_t glid = glCreateShader(stage);

    // Set the shader source
    glShaderSource(glid, 1, &sourcePtr, nullptr);

    // Compile the shader
    glCompileShader(glid);

    // Check for compilation errors
    int success;
    glGetShaderiv(glid, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(glid, 512, nullptr, infoLog);
        printf("Shader compilation error:\n%s", infoLog);

        return tl::nullopt;
    }

    auto [it, wasEmplaced] = m_shaderMap.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(pathSid),
      std::forward_as_tuple(pathSid, glid, source));

    if (wasEmplaced) { return &it->second; } // TODO insert check/warning
    else { return &m_shaderMap.find(pathSid)->second; }
}

ShaderLibrary_s g_shaderLibrary;

} // namespace cge
