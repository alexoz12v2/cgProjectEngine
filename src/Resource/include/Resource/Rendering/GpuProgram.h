#pragma once

#include "Core/Containers.h"
#include "Core/Type.h"

#include <span>

namespace cge
{

inline U32_t constexpr maxShaderProgramCount   = 13;
inline U32_t constexpr maxUniformPropertyCount = 16;

struct ShaderSpec_t
{
    Sid_t          sid;
    U32_t          stage;
    Char8_t const *source;
};

struct UniformBlockSpec_t
{
    Sid_t           program;
    Char8_t const  *blockName;
    Char8_t const **uniformNames;
    U32_t           uniformCount;
};

struct UniformBlockOut_t
{
    I32_t uniformOffset[maxUniformPropertyCount];
    I32_t uniformSize[maxUniformPropertyCount];
    I32_t uniformType[maxUniformPropertyCount];
    U32_t uniformCount;

    U32_t blockIdx;
    I32_t blockSize;
};

class GpuProgram_s
{
  public:
    GpuProgram_s();
    ~GpuProgram_s();

    void bind() const;
    void unbind() const;

    // TODO add support for custom syntax to create pipelines in one shader
    // file. ( __[stuff in here]__ )
    void addShader(ShaderSpec_t const &spec);
    // TODO take sid
    void addProgram(U32_t id); // program built from SPIR-V shaders

    // problem: if you need them in multiple places you need to set them
    // multiple times
    UniformBlockOut_t getUniformBlock(UniformBlockSpec_t const &spec) const;

    // done once you added all shaders and programs, and added all their
    // samplers and uniforms
    // refer to
    // https://www.khronos.org/opengl/wiki/Shader_Compilation#Program_pipelines
    void useStages(
      std::initializer_list<Sid_t> const &programs,
      std::initializer_list<U32_t> const &stagesPerProgram) const;

    // add support for uniform blocks and uniform buffer objects
    U32_t id(Sid_t sid) const;

  private:
    U32_t findProgram(Sid_t sid) const;
    struct Shader_t
    {
        Sid_t sid;
        U32_t glid;
        U32_t mask;
    };
    Array<Shader_t, maxShaderProgramCount> m_shaders{};

    U32_t m_shaderCount = 0;
    U32_t m_id          = 0;
};
} // namespace cge