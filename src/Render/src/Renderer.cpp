#include "Renderer.h"


namespace cge
{
glm::mat4 Camera_t::viewTransform() const
{
    return glm::lookAt(position, position + forward, up);
}
} // namespace cge
