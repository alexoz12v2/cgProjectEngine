#pragma once

#include "Core/Containers.h"
#include "Core/Type.h"
#include "ShaderLibrary.h"

#include <span>

namespace cge
{

inline U32_t constexpr maxShaderProgramCount   = 13;
inline U32_t constexpr maxUniformPropertyCount = 16;

struct UniformBlockSpec_t
{
    Char8_t const        *blockName;
    Char8_t const *const *uniformNames;
    U32_t                 uniformCount;
};

struct ProgramSpec_t
{
    Sid_t           sid;
    Char8_t const **pSources;
    U32_t          *pStages;
    U32_t           sourcesCount;
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

    void build(ProgramSpec_t const &specs);
    void build(Char8_t const *name, Shader_s const ** pShaders, U32_t count);

    // problem: if you need them in multiple places you need to set them
    // multiple times
    UniformBlockOut_t getUniformBlock(UniformBlockSpec_t const &spec) const;

    // add support for uniform blocks and uniform buffer objects
    U32_t id() const { return m_id; };

  private:
    U32_t m_id = 0;
    Sid_t m_sid;
};
} // namespace cge