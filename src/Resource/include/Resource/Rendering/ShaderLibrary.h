#pragma once

#include "Core/Module.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"

#include <map>
#include <string>
#include <tl/optional.hpp>

namespace cge
{

struct Shader_s
{
    Sid_t            sid;
    U32_t            glid;
    std::pmr::string source;
};

class ShaderLibrary_s
{
  public:
    tl::optional<Shader_s *> open(char const *);

  private:
    std::pmr::map<Sid_t, Shader_s> m_shaderMap{ getMemoryPool() };
};

extern ShaderLibrary_s g_shaderLibrary;

} // namespace cge
