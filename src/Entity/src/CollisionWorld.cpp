#include "CollisionWorld.h"

#include "Resource/HandleTable.h"
#include "Resource/Rendering/Mesh.h"

namespace cge
{
B8_t intersectObj(Ray_t const &ray, CollisionObj_t const &obj, Hit_t &outHit);

CollisionWorld_s::CollisionWorld_s(
  std::pmr::vector<BVHNode_t>      nodes,
  std::pmr::vector<CollisionObj_t> objects)
  : nodes(std::move(nodes)), objs(std::move(objects))
{
    this->nodes.reserve(maxCap);
}

void CollisionWorld_s::build()
{
    static U32_t constexpr rootIdx = 0;

    BVHNode_t &root      = nodes[rootIdx];
    root.children[0]     = nullIdx;
    root.children[1]     = nullIdx;
    root.primitivesCount = (U32_t)objs.size();
    root.firstPrimOffset = 0;
    updateNodeBounds(rootIdx);

    nodesUsed = 1;
    subdivide(rootIdx);
}

bool CollisionWorld_s::intersect(Ray_t const &ray, U32_t nodeIdx, Hit_t &outHit)
  const
{
    // Phase 1: Terminate if the ray misses the AABB of this node
    BVHNode_t const &node = nodes[nodeIdx];
    if (!testOverlap(ray, node.ebox)) { return false; }

    if (node.isLeaf())
    {
        // Phase 2.a: If the node is a leaf, intersect the object inside
        return intersectLeaf(
          ray, node.firstPrimOffset, node.primitivesCount, outHit);
    }
    else
    {
        // Phase 2.b: if the node is internal, recorse into left and right child
        // outHit will be modified only if we found a nearer hit
        return intersect(ray, node.children[0], outHit)
               || intersect(ray, node.children[1], outHit);
    }
}

void CollisionWorld_s::addObject(CollisionObj_t const &obj)
{
    objs.push_back(obj);
}

void CollisionWorld_s::subdivide(U32_t nodeIdx)
{
    BVHNode_t &node = nodes[nodeIdx];

    // retursion termination
    if (node.primitivesCount <= maxPrimsInNode) { return; }

    // Phase 1: split AABB across longest axis
    glm::vec3 extent = diagonal(node.ebox);
    U32_t     axis   = 0;
    if (extent.y > extent.x) { axis = 1; }
    if (extent.z > extent[axis]) { axis = 2; }
    F32_t splitPos = node.ebox.min[axis] + extent[axis] * 0.5f;

    // Phase 2: split the group in 2 halves
    U32_t i = node.firstPrimOffset;
    U32_t j = i + node.primitivesCount - 1;
    while (i <= j)
    {
        if (centroid(objs[i].ebox)[axis] < splitPos) { i++; }
        else { std::swap(objs[i], objs[j--]); }
    }

    // Phase 3: create child nodes for each half
    U32_t leftCount = i - node.firstPrimOffset;
    if (leftCount == 0 || leftCount == node.primitivesCount)
    {
        // empty box either to the left or right
    }
    else
    {
        U32_t leftChildIdx  = nodesUsed++;
        U32_t rightChildIdx = nodesUsed++;
        node.children[0]    = leftChildIdx;
        node.children[1]    = rightChildIdx;

        nodes[leftChildIdx].firstPrimOffset  = node.firstPrimOffset;
        nodes[leftChildIdx].primitivesCount  = leftCount;
        nodes[rightChildIdx].firstPrimOffset = i;
        nodes[rightChildIdx].primitivesCount = node.primitivesCount - leftCount;

        node.primitivesCount = 0; // this is internal

        updateNodeBounds(leftChildIdx);
        updateNodeBounds(rightChildIdx);

        // Phase 4: recurse into child nodes
        subdivide(leftChildIdx);
        subdivide(rightChildIdx);
    }
}

void CollisionWorld_s::updateNodeBounds(U32_t nodeIdx)
{
    //
    BVHNode_t &node = nodes[nodeIdx];

    node.ebox = { .min = glm::vec3(std::numeric_limits<float>::max()),
                  .max = glm::vec3(std::numeric_limits<float>::epsilon()) };
    for (U32_t objIdx = node.firstPrimOffset, i = 0; i != node.primitivesCount;
         ++i)
    {
        node.ebox = aUnion(node.ebox, objs[objIdx + i].ebox);
    }
}
B8_t CollisionWorld_s::intersectLeaf(
  Ray_t const &ray,
  U32_t        firstObjIdx,
  U32_t        objCount,
  Hit_t       &outHit) const
{
    std::span primsInNode(&objs[firstObjIdx], objCount);
    B8_t      bIsect = false;
    for (U32_t i = 0; i != primsInNode.size(); i++)
    {
        Hit_t hit;
        bool  currentIsect = intersectObj(ray, primsInNode[i], hit);
        if (currentIsect && hit.t < (outHit.t + 0.01f)) { outHit = hit; }
        bIsect = bIsect || currentIsect;
    }

    return bIsect;
}

// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
B8_t intersectObj(Ray_t const &ray, CollisionObj_t const &obj, Hit_t &outHit)
{
    // get a hold of the mesh
    auto ref   = g_handleTable.get(obj.sid);
    bool found = false;
    if (ref.hasValue())
    {
        for (Mesh_s const &mesh = ref.getAs<Mesh_s>();
             auto const   &face : mesh.indices)
        {
            glm::vec3 v0 =
              mesh.transform * glm::vec4(mesh.vertices[face[0]].pos, 1.f);
            glm::vec3 v1 =
              mesh.transform * glm::vec4(mesh.vertices[face[1]].pos, 1.f);
            glm::vec3 v2 =
              mesh.transform * glm::vec4(mesh.vertices[face[2]].pos, 1.f);

            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 h     = glm::cross(ray.d, edge2);
            float     angle = glm::dot(h, edge1);

            // is the ray parallel to the triangle?
            if (angle > -0.0001f && angle < 0.0001f) { continue; }

            float     f = 1 / angle;
            glm::vec3 s = ray.o - v0;
            float     u = f * glm::dot(s, h);
            if (u < 0 || u > 1) { continue; }

            glm::vec3 q = glm::cross(s, edge1);
            if (float v = f * glm::dot(ray.d, q); v < 0 || u + v > 1)
            {
                continue;
            }

            if (float t = f * glm::dot(edge2, q); t > 0.0001f && outHit.t > t)
            {
                outHit.objId = obj.sid;
                outHit.t     = t;
                outHit.p     = ray.o + ray.d * t;
                found        = true;
            }
        }
        return found;
    }
}

} // namespace cge