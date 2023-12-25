#pragma once

#include "Core/StringUtils.h"
#include "Core/Type.h"

#include <span>
#include <vector>

// collision world is a BVH containing Sids which all index into an array of
// collision bodies through tables, which are simplified meshes associated with
// complex meshes, all enclused in a bvh

// all objects will have 2 AABBs and 2 meshes associated with it: a conventional
// AABB, and an enlarged one (stored as Vec3 Offset + float enlargment in the
// object, here fully). The object will be removed and reinserted in the tree if
// the inner AABB overlaps the outer one
namespace cge
{
struct CollisionObj_t
{
    AABB_t ebox;
    Sid_t  sid;
};

struct Hit_t
{
    Sid_t     objId = nullSid;
    glm::vec3 p{ -1.f };
    F32_t     t = std::numeric_limits<float>::max();
};

// to access child, use objs[firstPrimOffset]
struct BVHNode_t
{
    AABB_t ebox;
    U32_t  children[2];
    U32_t  firstPrimOffset;
    U32_t  primitivesCount; // if 0 then interior, otherwise leaf
    bool   isLeaf() const { return primitivesCount != 0; }
};

class CollisionWorld_s
{
  public:
    static U32_t constexpr nullIdx        = (U32_t)-1;
    static U32_t constexpr maxCap         = 1024;
    static U32_t constexpr maxPrimsInNode = 2u;

    CollisionWorld_s();

    void addObject(CollisionObj_t const &obj);

    // maybe later, now we'll simply rebuild it each frame
    // void removeObject(Sid_t obj);
    // void refit();

    // assumes outHit is default constructed
    bool intersect(Ray_t const &ray, U32_t objIdx, Hit_t &outHit) const;
    void build();

  private:
    void subdivide(U32_t nodeIdx);
    void updateNodeBounds(U32_t nodeIdx);
    bool intersectLeaf(
      Ray_t const &ray,
      U32_t        firstObjIdx,
      U32_t        objCount,
      Hit_t       &outHit) const;

    // polymorphic memory res will be constructed from a big chunk of the
    // stackAllocator, hoping to never have to reallocate
    // the capacity is checked at every insertion
    // node[0] = root
    std::vector<BVHNode_t> m_bnodes;
    U32_t                  nodesUsed = 0;

    // collects updates and then distirbutes to nodes?
    std::vector<CollisionObj_t> objs;
};

extern CollisionWorld_s g_world;

} // namespace cge
