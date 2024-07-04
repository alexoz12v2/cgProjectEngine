#ifndef CGE_PLAYER_H
#define CGE_PLAYER_H

#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Entity/CollisionWorld.h"
#include "Render/Renderer.h"
#include "Resource/HandleTable.h"

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>

#include <unordered_set>

namespace cge
{

class Player
{
  public:
    static F32_t constexpr baseVelocity     = 100.F;
    static F32_t constexpr mouseSensitivity = 0.1F;

    void spawn(Camera_t const &, Sid_t);

    void onKey(I32_t key, I32_t action);
    void onMouseButton(I32_t key, I32_t action);
    void onMouseMovement(F32_t xPos, F32_t yPos);
    void onTick(F32_t deltaTime);

    [[nodiscard]] AABB_t    boundingBox() const;
    [[nodiscard]] glm::mat4 viewTransform() const;
    [[nodiscard]] Camera_t  getCamera() const;
    [[nodiscard]] glm::vec3 getCentroid() const;

    // TODO remove
    [[nodiscard]] glm::vec3 lastDisplacement() const;

  private:
    void      yawPitchRotate(F32_t yaw, F32_t pitch);
    AABB_t    recomputeGlobalSpaceBB() const;
    glm::vec3 displacementTick(F32_t deltaTime) const;

    // main components
    Sid_t                m_sid;
    HandleTable_s::Ref_s m_mesh = nullRef;
    Camera_t             m_camera{};

    // collision related
    AABB_t                      m_box;
    CollisionWorld_s::ObjHandle m_worldObjPtr;

    // movement related
    glm::ivec2 m_keyPressed{ 0, 0 }; // WS AD
    glm::vec2  m_lastCursorPosition{ -1.0F, -1.0F };
    B8_t       m_isCursorDisabled = false;
    glm::vec3  m_lastDisplacement{};
};

class ScrollingTerrain
{
  public:
    static U32_t constexpr pieceSize = 100;

    /**
     * @fn init
     * @brief assuming each piece is 10 m x 10 m, they are placed such that the
     * middle one is in the origin of the world
     *
     * @param scene scene in which the objects are instanced
     * @param begin begin iterator for the pieces of the terrain. Can be
     * obtained with Scene_s::names()
     * @param end end iterator for the pieces of the terrain. Can be obtained
     * with Scene_s::namesEnd()
     */
    void init(
      Scene_s                                &scene,
      std::pmr::vector<Sid_t>::const_iterator begin,
      std::pmr::vector<Sid_t>::const_iterator end);

    /**
     * @fn updateTilesFromPosition
     * @brief given the player position, find in which of the tiles the player
     * is into, and, if it isn't the one in the center, move the tiles in the
     * back forward such that the player will be in the center. Also, randomly
     * pick a new mesh sid for a new piece
     *
     * @param position current player position
     * @param sidSet set of meshes to pick from
     */
    void updateTilesFromPosition(
      glm::vec3                      position,
      std::pmr::vector<Sid_t> const &sidSet);

  private:
    // the front is the furthest piece backwards, hence the first to be moved
    // and swapped with the back
    std::pmr::vector<SceneNode_s *> m_pieces;
};

} // namespace cge

#endif // CGE_PLAYER_H
