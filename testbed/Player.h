#pragma once

#include "Core/Event.h"
#include "Core/Random.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Render/Renderer.h"
#include "Resource/HandleTable.h"

#include "SoundEngine.h"

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>

#include <deque>
#include <unordered_set>

namespace cge
{

inline U32_t constexpr pieceSize             = 100;
inline U32_t constexpr halfPieceSize         = 50;
inline U32_t constexpr numLanes              = 3;
inline F32_t constexpr laneShift             = (F32_t)pieceSize / numLanes;
inline F32_t constexpr coinPositionIncrement = static_cast<F32_t>(pieceSize * 5);

class ScrollingTerrain
{
  private:
    static U32_t constexpr numPieces          = 10;
    static U32_t constexpr maxDestructables   = 4;
    static U32_t constexpr maxObstacles       = 4;
    static U32_t constexpr maxPieces          = 4;
    static U32_t constexpr maxNumSpawnedCoins = 20;

  public:
    using ObstacleList = std::array<Sid_t, numPieces>;
    using PieceList    = std::array<Sid_t, numPieces>;
    using CoinMap      = std::pmr::unordered_map<U32_t, Sid_t>;
    struct InitData
    {
        std::span<Sid_t> pieces;
        std::span<Sid_t> obstacles;
        std::span<Sid_t> destructables;
        Sid_t            magnetPowerUp;
        Sid_t            coin;
    };

  public:
    void init(InitData const &initData);
    void updateTilesFromPosition(glm::vec3 position);
    void handleShoot(Ray const &ray);

    ObstacleList const &getObstacles() const;
    ObstacleList const &getDestructables() const;
    CoinMap const      &getCoinMap() const;
    void                removeCoin(CoinMap::iterator const &it);
    void                removeCoin(CoinMap::const_iterator const &it);

  private:
    Sid_t selectRandomPiece() const;
    Sid_t selectRandomObstacle() const;
    Sid_t selectRandomDestructable() const;

    void addPropOfType(U32_t type, glm::mat4 const &pieceTransform);
    void addCoins(F32_t pieceYCoord);
    void removeCoins(F32_t pieceYCoord);

    template<U32_t N> static constexpr Sid_t selectRandomFromList(std::array<Sid_t, N> list, U32_t effectiveSize)
    {
        U32_t rnd = g_random.next<U32_t>(0, effectiveSize - 1);
        Sid_t ret{ nullSid };
        while (ret == nullSid && rnd >= 0)
        { //
            ret = list[rnd--];
        }
        assert(ret != nullSid && "[ScrollingTerrain] invalid");
        return ret;
    }

  private:
    // the front is the furthest piece backwards, hence the first to be moved
    // and swapped with the back
    PieceList    m_pieces{ nullSid };
    ObstacleList m_obstacles{ nullSid };
    ObstacleList m_destructables{ nullSid };

    // map from approximated y position -> scene sid of the coin
    CoinMap m_coinMap{ getMemoryPool() };

    F32_t m_coinYCoord{ coinPositionIncrement };

    // indices managed such that the m_pieces array is a ring buffer, with its
    // elements fixed and varying indices
    U32_t m_first{ 0 };

    // available meshes
    std::array<Sid_t, maxPieces>        m_pieceSet{ nullSid };
    std::array<Sid_t, maxObstacles>     m_obstacleSet{ nullSid };
    std::array<Sid_t, maxDestructables> m_destructableSet{ nullSid };
    Sid_t                               m_coin{ nullSid };
    Sid_t                               m_magnetPowerUp{ nullSid };

    U32_t m_pieceSetSize{ 0 };
    U32_t m_obstacleSetSize{ 0 };
    U32_t m_destructableSetSize{ 0 };
};

class Player
{
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

    [[nodiscard]] AABB      boundingBox() const;
    [[nodiscard]] glm::mat4 viewTransform() const;
    [[nodiscard]] Camera_t  getCamera() const;
    [[nodiscard]] glm::vec3 getCentroid() const;

    // TODO remove
    [[nodiscard]] glm::vec3 lastDisplacement() const;

    bool intersectPlayerWith(ScrollingTerrain const &terrain);
    void setSwishSound(irrklang::ISoundSource *sound);
    void incrementScore(U32_t increment);

  private:
    static F32_t constexpr scoreMultiplier        = 0.1f;
    static F32_t constexpr baseShiftVelocity      = 50.f;
    static F32_t constexpr baseVelocity           = 200.f;
    static F32_t constexpr maxBaseVelocity        = 400.f;
    static F32_t constexpr mouseSensitivity       = 0.1f;
    static F32_t constexpr multiplierTimeConstant = 0.1f;
    static U8_t constexpr LANE_LEFT               = 1 << 2; // 0000'0100
    static U8_t constexpr LANE_CENTER             = 1 << 1; // 0000'0010
    static U8_t constexpr LANE_RIGHT              = 1 << 0; // 0000'0001

  private:
    void      yawPitchRotate(F32_t yaw, F32_t pitch);
    glm::vec3 displacementTick(F32_t deltaTime) const;

  private:
    // main components
    Sid_t    m_sid{ nullSid };
    Mesh_s  *m_mesh{ nullptr };
    Camera_t m_camera{};
    U64_t    m_score{ 0 };
    B8_t     m_init{ false };

    // collision related
    bool      m_intersected{ false };
    glm::vec3 m_oldPosition{ 0.f };

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

    // sound data
    irrklang::ISoundSource *m_swishSoundSource{ nullptr };
    irrklang::ISound       *m_swishSound{ nullptr };
};

} // namespace cge
