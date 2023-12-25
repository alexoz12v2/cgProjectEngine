#include "Rendering/cgeMesh.h"

namespace cge
{
AABB_t computeAABB(const Mesh_s &mesh)
{
    if (mesh.vertices.empty())
    {
        // Handle the case when the mesh has no vertices
        // You might want to return an AABB with some default values or handle
        // it differently
        return AABB_t{ glm::vec3(0.0f), glm::vec3(0.0f) };
    }

    // Initialize the min and max coordinates with the first vertex
    glm::vec3 minCoord = mesh.vertices[0].pos;
    glm::vec3 maxCoord = mesh.vertices[0].pos;

    // Iterate through all vertices to find the min and max coordinates
    for (const auto &vertex : mesh.vertices)
    {
        // Update min coordinates
        minCoord = glm::min(minCoord, vertex.pos);

        // Update max coordinates
        maxCoord = glm::max(maxCoord, vertex.pos);
    }

    return AABB_t{ minCoord, maxCoord };
}
} // namespace cge
