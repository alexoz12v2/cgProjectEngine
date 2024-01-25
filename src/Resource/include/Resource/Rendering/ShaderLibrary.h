//
// Created by alessio on 07/01/24.
//

#ifndef CGE_SHADERLIBRARY_H
#define CGE_SHADERLIBRARY_H

#include "Core/StringUtils.h"
#include "Core/Type.h"

#include <map>
#include <string>
#include <tl/optional.hpp>

namespace cge
{

struct Shader_s
{
    Sid_t       sid;
    U32_t       glid;
    std::string source;
};

class ShaderLibrary_s
{
  public:
    tl::optional<Shader_s *> open(char const *);

  private:
    std::map<Sid_t, Shader_s> m_shaderMap;
};

extern ShaderLibrary_s g_shaderLibrary;

} // namespace cge

#endif // CGE_SHADERLIBRARY_H
