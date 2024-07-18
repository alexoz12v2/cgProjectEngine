#ifndef CGE_PLAYER_H
#define CGE_PLAYER_H

#include "Core/Event.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Entity/CollisionWorld.h"
#include "Render/Renderer.h"
#include "Resource/HandleTable.h"

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>

#include <deque>
#include <unordered_set>

namespace cge
{

inline U32_t constexpr pieceSize = 100;
inline U32_t constexpr numLanes  = 3;
inline F32_t constexpr laneShift = (F32_t)pieceSize / numLanes;

class Player
{
  public:
    static F32_t constexpr scoreMultiplier        = 0.1f;
    static F32_t constexpr baseShiftVelocity      = 50.f;
    static F32_t constexpr baseVelocity           = 200.f;
    static F32_t constexpr maxBaseVelocity        = 400.f;
    static F32_t constexpr mouseSensitivity       = 0.1f;
    static F32_t constexpr multiplierTimeConstant = 0.1f;
    static U8_t constexpr LANE_LEFT               = 1 << 2; // 0000'0100
    static U8_t constexpr LANE_CENTER             = 1 << 1; // 0000'0010
    static U8_t constexpr LANE_RIGHT              = 1 << 0; // 0000'0001

  public:
    Player()                                   = default;
    Player(Player const &other)                = default;
    Player &operator=(Player const &other)     = default;
    Player(Player &&other) noexcept            = default;
    Player &operator=(Player &&other) noexcept = default;
    ~Player();

  public:
    void spawn(Camera_t const &, Sid_t);

    void onKey(I32_t key, I32_t action);
    void onMouseButton(I32_t key, I32_t action);
    void onTick(F32_t deltaTime);
    void onFramebufferSize(I32_t width, I32_t height);

    [[nodiscard]] AABB_t    boundingBox() const;
    [[nodiscard]] glm::mat4 viewTransform() const;
    [[nodiscard]] Camera_t  getCamera() const;
    [[nodiscard]] glm::vec3 getCentroid() const;

    // TODO remove
    [[nodiscard]] glm::vec3 lastDisplacement() const;

    bool intersectPlayerWith(
      std::pmr::deque<std::array<SceneNode_s *, numLanes>> const &obstacles,
      Hit_t                                                      &outHit);

  private:
    void      yawPitchRotate(F32_t yaw, F32_t pitch);
    glm::vec3 displacementTick(F32_t deltaTime) const;

  private:
    // main components
    Sid_t                m_sid{ nullSid };
    SceneNode_s         *m_node{ nullptr };
    HandleTable_s::Ref_s m_mesh{ nullRef };
    Camera_t             m_camera{};
    U64_t                m_score{ 0 };
    B8_t                 m_init{ false };

    // collision related
    AABB_t                      m_box;
    CollisionWorld_s::ObjHandle m_worldObjPtr;
    bool                        m_intersected{ false };

    // movement related
    U8_t      m_lane{ LANE_CENTER };
    F32_t     m_targetXPos{ 0.f };
    F32_t     m_velocityMultiplier{ 1.f };
    glm::vec3 m_lastDisplacement{};

    // event data
    glm::uvec2 m_framebufferSize{ 0, 0 };
    union
    {
        struct
        {
            std::pair<Event_t, Sid_t> keyListener;
            std::pair<Event_t, Sid_t> mouseButtonListener;
            std::pair<Event_t, Sid_t> framebufferSizeListener;
        };
        std::array<std::pair<Event_t, Sid_t>, 3> arr;
    } m_listeners{};
};

class ScrollingTerrain
{
  public:
    /**
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
     * @brief given the player position, find in which of the tiles the player
     * is into, and, if it isn't the one in the center, move the tiles in the
     * back forward such that the player will be in the center. Also, randomly
     * pick a new mesh sid for a new piece
     *
     * @param position current player position
     * @param sidSet set of meshes to pick from
     * @return deque with obstacles for each tile (hence size = m_pieces.size())
     */
    std::pmr::deque<std::array<SceneNode_s *, numLanes>>
      updateTilesFromPosition(
        glm::vec3                      position,
        std::pmr::vector<Sid_t> const &sidSet,
        std::pmr::vector<Sid_t> const &obstacles);

  private:
    // the front is the furthest piece backwards, hence the first to be moved
    // and swapped with the back
    std::pmr::vector<SceneNode_s *>                      m_pieces;
    std::pmr::deque<std::array<SceneNode_s *, numLanes>> m_obstacles;
};

} // namespace cge

#endif // CGE_PLAYER_H
