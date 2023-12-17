#pragma once

#include "Core/Containers.h"
#include "Core/Type.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace cge
{

struct Camera_t
{
    glm::mat4 viewTransform() const;

    glm::vec3 forward;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 position;
};

struct Light_t
{
    //
};

} // namespace cge
