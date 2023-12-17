#include "Rendering/GpuProgram.h"

#include "RenderUtils/GLutils.h"

#include <glad/gl.h>

#include <algorithm>

namespace cge
{


GLenum bitFromType(GLenum);

GpuProgram_s::GpuProgram_s()
{
    GL_CHECK(glGenProgramPipelines(1, &m_id));
    GL_CHECK(glBindProgramPipeline(m_id));
}

GpuProgram_s::~GpuProgram_s()
{
    for (U32_t i = 0; i != m_shaderCount; ++i)
    {
        glDeleteProgram(m_shaders[i].glid);
    }
    GL_CHECK(glDeleteProgramPipelines(1, &m_id));
}

void GpuProgram_s::bind() const
{
    GL_CHECK(glUseProgram(0)); // useprogram overrides pipeline
    GL_CHECK(glBindProgramPipeline(m_id));
}

void GpuProgram_s::unbind() const { GL_CHECK(glBindProgramPipeline(0)); }

// add some error handling
// TODO add fucntions should return an identifier
void GpuProgram_s::addShader(ShaderSpec_t const &spec)
{
    assert(
      m_shaderCount + 1 <= maxShaderProgramCount
      && "max shader program count exceeded");
#if defined(CGE_DEBUG)
    auto glidFunc = [&spec]()
    {
        U32_t id;
        GL_CHECK(id = glCreateShaderProgramv(spec.stage, 1, &spec.source));
        return id;
    };
#endif

    // compile and partially link in one shot
    m_shaders[m_shaderCount++] = {
        .sid = spec.sid,
#if defined(CGE_DEBUG)
        .glid = glidFunc(),
#else
        .glid = glCreateShaderProgramv(spec.stage, 1, &spec.source),
#endif
        .mask = bitFromType(spec.stage),
    };
} // namespace cge

// add some error handling
void GpuProgram_s::addProgram(U32_t id)
{
    // first make the program separable
    GL_CHECK(glProgramParameteri(id, GL_PROGRAM_SEPARABLE, GL_TRUE));
}

UniformBlockOut_t
  GpuProgram_s::getUniformBlock(UniformBlockSpec_t const &spec) const
{
    UniformBlockOut_t out;
    assert(spec.uniformCount <= maxUniformPropertyCount);
    U32_t uniformIndices[maxUniformPropertyCount];

    // get Uniform Block
    U32_t pid = id(spec.program);
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

void GpuProgram_s::useStages(
  std::initializer_list<Sid_t> const &programs,
  std::initializer_list<U32_t> const &stagesPerProgram) const
{
    assert(
      stagesPerProgram.size() <= m_shaderCount
      && programs.size() == stagesPerProgram.size());

    auto stagesIt = stagesPerProgram.begin();
    for (auto const program : programs)
    {
        if (U32_t idx = findProgram(program); idx != (U32_t)-1)
        {
            Shader_t const &prog = m_shaders[idx];
            U32_t const     mask = *stagesIt;
            assert((prog.mask & mask) != 0 && "stages missing in program");
            GL_CHECK(glUseProgramStages(m_id, mask, prog.glid));
        }
        else { std::printf("program not found"); }
        ++stagesIt;
    }

#if 0
    auto stagesIt = stagesPerProgram.begin();
    for (auto progIt = programs.begin(); progIt != programs.end(); ++progIt)
    {
        auto const stage = *stagesIt;
        auto const mask  = m_shaderMasks[*progIt];
        assert((stage & mask) != 0 && "stages missing in program");

        GL_CHECK(glUseProgramStages(m_id, *stagesIt, m_shaders[*progIt]));

        ++stagesIt;
    }
#endif
}

U32_t GpuProgram_s::id(Sid_t sid) const
{
    return m_shaders[findProgram(sid)].glid;
}

U32_t GpuProgram_s::findProgram(Sid_t sid) const
{
    auto it = std::ranges::find_if(
      m_shaders, [sid](auto const &shader) { return shader.sid == sid; });
    if (it == m_shaders.cend()) { return (U32_t)-1; }
    else { return (U32_t)std::distance(m_shaders.cbegin(), it); }
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
