#pragma once

#include "Core/Type.h"

namespace cge
{

struct Ray
{
  public:
    Ray(glm::vec3 const &orig, glm::vec3 const &dir)
      : orig(orig), dir(dir), invdir(1.f / dir), sign{ (invdir.x < 0), (invdir.y < 0), (invdir.z < 0) }
    {
    }

  public:
    glm::vec3 orig, dir; // Ray origin and direction
    glm::vec3 invdir;
    I8_t      sign[3];
};

struct Hit_t
{
    glm::vec3 p;
    F32_t     t;
    B8_t      isect;
};

union AABB
{
    AABB(glm::vec3 const &min_, glm::vec3 const &max_) : mm(min_, max_) {}

    struct S
    {
        glm::vec3 min;
        glm::vec3 max;
    };
    S mm;
    glm::vec3 bounds[2];
};


// preliminary intersection test on AABB
B8_t testOverlap(Ray const &ray, AABB const &box);

Hit_t intersect(Ray const &ray, AABB const &box);

AABB aUnion(AABB const &a, AABB const &b);

// 00 -> x, 01 -> y, 10 -> z
I32_t largestAxis(AABB const &box, F32_t *plane);

F32_t aArea(AABB const &a);

glm::vec3 centroid(AABB const &b);

glm::vec3 diagonal(AABB const &b);

B8_t isPointInsideAABB(glm::vec3 const &point, AABB const &box);

AABB transformAABBToNDC(AABB const &aabb, glm::mat4 const &model, glm::mat4 const &view, glm::mat4 const &projection);

} // namespace cge
