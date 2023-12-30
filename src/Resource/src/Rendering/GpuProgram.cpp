#include "Rendering/GpuProgram.h"

#include "RenderUtils/GLutils.h"

#include <glad/gl.h>

#include <algorithm>

namespace cge
{


GLenum bitFromType(GLenum);

GpuProgram_s::GpuProgram_s()
{
    //
    GL_CHECK(m_id = glCreateProgram());
}

GpuProgram_s::~GpuProgram_s()
{
    //
    glDeleteProgram(m_id);
}

void GpuProgram_s::bind() const { GL_CHECK(glUseProgram(m_id)); }

void GpuProgram_s::unbind() const { GL_CHECK(glUseProgram(0)); }

void GpuProgram_s::build(ProgramSpec_t const &specs)
{
    U32_t static shaderBuffer[32]{};

    GLint  success;
    GLchar infoLog[512];

    m_sid = specs.sid;
    for (U32_t i = 0; i != specs.sourcesCount; i++)
    {
        shaderBuffer[i] = glCreateShader(specs.pStages[i]);
        glShaderSource(shaderBuffer[i], 1, &specs.pSources[i], nullptr);
        glCompileShader(shaderBuffer[i]);
        glGetShaderiv(shaderBuffer[i], GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shaderBuffer[i], 512, nullptr, infoLog);
            printf("%s", infoLog);
        }
        glAttachShader(m_id, shaderBuffer[i]);
    }

    glLinkProgram(m_id);
    glGetProgramiv(m_id, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(m_id, 512, nullptr, infoLog);
        printf("%s", infoLog);
    }

    for (U32_t i = 0; i != specs.sourcesCount; i++)
    {
        glDeleteShader(shaderBuffer[i]);
    }
}

UniformBlockOut_t
  GpuProgram_s::getUniformBlock(UniformBlockSpec_t const &spec) const
{
    UniformBlockOut_t out;
    assert(spec.uniformCount <= maxUniformPropertyCount);
    U32_t uniformIndices[maxUniformPropertyCount];

    // get Uniform Block
    U32_t pid = m_id;
    GL_CHECK(out.blockIdx = glGetUniformBlockIndex(pid, spec.blockName));
    assert(out.blockIdx != GL_INVALID_INDEX);
    GL_CHECK(glGetActiveUniformBlockiv(
      pid, out.blockIdx, GL_UNIFORM_BLOCK_DATA_SIZE, &out.blockSize));

    // get uniforms indices
    GL_CHECK(glGetUniformIndices(
      pid, spec.uniformCount, spec.uniformNames, uniformIndices));

    // get indices properties
    GL_CHECK(glGetActiveUniformsiv(
      pid,
      spec.uniformCount,
      uniformIndices,
      GL_UNIFORM_OFFSET,
      out.uniformOffset));
    GL_CHECK(glGetActiveUniformsiv(
      pid,
      spec.uniformCount,
      uniformIndices,
      GL_UNIFORM_TYPE,
      out.uniformType));
    GL_CHECK(glGetActiveUniformsiv(
      pid,
      spec.uniformCount,
      uniformIndices,
      GL_UNIFORM_SIZE,
      out.uniformSize));

    return out;
}

GLenum bitFromType(GLenum type)
{
    switch (type)
    {
    case GL_VERTEX_SHADER:
        return GL_VERTEX_SHADER_BIT;
    case GL_TESS_CONTROL_SHADER:
        return GL_TESS_CONTROL_SHADER_BIT;
    case GL_TESS_EVALUATION_SHADER:
        return GL_TESS_EVALUATION_SHADER_BIT;
    case GL_GEOMETRY_SHADER:
        return GL_GEOMETRY_SHADER_BIT;
    case GL_FRAGMENT_SHADER:
        return GL_FRAGMENT_SHADER_BIT;
    case GL_COMPUTE_SHADER:
        return GL_COMPUTE_SHADER_BIT;
    default:
        return GL_ALL_SHADER_BITS;
    }
}
} // namespace cge
