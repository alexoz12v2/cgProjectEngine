#pragma once

#include "Core/Containers.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"

#include <glm/ext/vector_float3.hpp>

namespace cge
{

struct Light_t
{
    Sid_t sid;

    struct Properties
    {
        B8_t isEnabled; // true to apply this light in this invocation
        B8_t isLocal;   // true - point light or a spotlight, false - positional
                        // light
        B8_t      isSpot;     // true if the light is a spotlight
        glm::vec3 ambient;    // light’s contribution to ambient light
        glm::vec3 color;      // color of light
        glm::vec3 position;   // isLocal - light location, else light direction
        glm::vec3 halfVector; // direction of highlights for directional light
        glm::vec3 coneDirection; // spotlight attributes
        F32_t     spotCosCutoff;
        F32_t     spotExponent;
        F32_t     constantAttenuation; // local light attenuation coefficients
        F32_t     linearAttenuation;
        F32_t     quadraticAttenuation;
    };

    Properties props;
};
} // namespace cge
