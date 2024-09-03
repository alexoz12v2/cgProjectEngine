#pragma once

#include "Core/Containers.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"

#include <glm/ext/vector_float3.hpp>

namespace cge
{

struct alignas(16) Light_t
{
    glm::vec3 ambient;       // light's contribution to ambient light
    alignas(16) glm::vec3 color;         // color of light
    alignas(16) glm::vec3 position;      // isLocal - light location, else light direction
    alignas(16) glm::vec3 halfVector;    // direction of highlights for directional light
    alignas(16) glm::vec3 coneDirection; // spotlight attributes
    F32_t     spotCosCutoff;
    F32_t     spotExponent;
    F32_t     constantAttenuation; // local light attenuation coefficients
    F32_t     linearAttenuation;
    F32_t     quadraticAttenuation;
    alignas(4) B8_t      isEnabled; // true to apply this light in this invocation
    alignas(4) B8_t      isLocal;   // true - point light or a spotlight, false - positional light
    alignas(4) B8_t      isSpot;    // true if the light is a spotlight
    unsigned char _padding[4];
};
static_assert(std::is_trivial_v<Light_t>, "what");
} // namespace cge
