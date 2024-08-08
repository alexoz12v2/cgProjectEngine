#include "Utility.h"

namespace cge
{

B8_t testOverlap(Ray const &ray, AABB const &box)
{
    // for each dimension, ray plane intersection
    for (int i = 0; i < 3; i++)
    {
        // if ray is parallel to the slab...
        if (glm::abs(ray.dir[i]) < std::numeric_limits<F32_t>::epsilon())
        {
            // ... and ray coord is not within, no hit
            // does this work with NaN?
            if (ray.orig[i] < box.min[i] || ray.orig[i] > box.max[i]) { return false; }
        }
    }
    return true;
}

Hit_t intersect(Ray const &r, AABB const &box)
{
    Hit_t hit{ .p{ 0.f, 0.f, 0.f }, .t{ -1.f }, .isect{ false } };
    F32_t tmin, tmax, tymin, tymax, tzmin, tzmax;

    tmin  = (box.bounds[r.sign[0]].x - r.orig.x) * r.invdir.x;
    tmax  = (box.bounds[1 - r.sign[0]].x - r.orig.x) * r.invdir.x;
    tymin = (box.bounds[r.sign[1]].y - r.orig.y) * r.invdir.y;
    tymax = (box.bounds[1 - r.sign[1]].y - r.orig.y) * r.invdir.y;

    if ((tmin <= tymax) && (tymin <= tmax))
    {
        if (tymin > tmin) //
            tmin = tymin;
        if (tymax < tmax) //
            tmax = tymax;

        tzmin = (box.bounds[r.sign[2]].z - r.orig.z) * r.invdir.z;
        tzmax = (box.bounds[1 - r.sign[2]].z - r.orig.z) * r.invdir.z;

        if ((tmin <= tzmax) && (tzmin <= tmax))
        {
            if (tzmin > tmin) //
                tmin = tzmin;
            if (tzmax < tmax) //
                tmax = tzmax;

            hit.isect = true;
            hit.t     = tmin;
            hit.p     = r.orig + tmin * r.dir;
        }
    }

    return hit;
}

AABB aUnion(AABB const &a, AABB const &b)
{
    AABB const c{ glm::min(a.min, b.min), glm::max(a.max, b.max) };
    return c;
}

I32_t largestAxis(AABB const &box, F32_t *plane)
{
    glm::vec3 diag = box.max - box.min;
    glm::vec3 mid  = (box.max + box.min) * 0.5f;
    int32_t   res  = 0;
    *plane         = mid.x;
    if (diag.x < diag.y)
    {
        res    = 1;
        *plane = mid.y;
        if (diag.y < diag.z)
        {
            *plane = mid.z;
            res    = 2;
        }
    }
    else if (diag.x < diag.z)
    {
        *plane = mid.z;
        res    = 2;
    }

    return res;
}

F32_t aArea(AABB const &a)
{
    glm::vec3 const d = a.max - a.min;
    return 2.f * (d.x * d.y + d.y * d.z + d.z * d.x);
}

glm::vec3 centroid(AABB const &b) { return (b.max + b.min) * 0.5f; }

glm::vec3 diagonal(AABB const &b) { return b.max - b.min; }

B8_t isPointInsideAABB(glm::vec3 const &point, AABB const &box)
{
    return (point.x >= box.min.x && point.x <= box.max.x) && (point.y >= box.min.y && point.y <= box.max.y)
           && (point.z >= box.min.z && point.z <= box.max.z);
}

AABB transformAABBToNDC(AABB const &aabb, glm::mat4 const &model, glm::mat4 const &view, glm::mat4 const &projection)
{
    // Define the 8 vertices of the AABB in model space
    glm::vec4 vertices[]{ { aabb.min, 1.0f },
                          { aabb.max, 1.0f },
                          { aabb.min.x, aabb.min.y, aabb.max.z, 1.0f },
                          { aabb.min.x, aabb.max.y, aabb.min.z, 1.0f },
                          { aabb.max.x, aabb.min.y, aabb.min.z, 1.0f },
                          { aabb.min.x, aabb.max.y, aabb.max.z, 1.0f },
                          { aabb.max.x, aabb.min.y, aabb.max.z, 1.0f },
                          { aabb.max.x, aabb.max.y, aabb.min.z, 1.0f } };

    // Combine model, view, and projection matrices
    glm::mat4 mvp = projection * view * model;

    // Transform vertices to clip space
    for (auto &vertex : vertices)
    {
        vertex = mvp * vertex;
        // Perform perspective divide to transform to NDC
        vertex /= vertex.w;
    }

    // Find the new min and max points in NDC space
    glm::vec3 ndc_min(std::numeric_limits<float>::max());
    glm::vec3 ndc_max(std::numeric_limits<float>::lowest());

    for (const auto &vertex : vertices)
    {
        ndc_min = glm::min(ndc_min, glm::vec3(vertex));
        ndc_max = glm::max(ndc_max, glm::vec3(vertex));
    }

    // Return the transformed AABB in NDC space
    return { ndc_min, ndc_max };
}
} // namespace cge