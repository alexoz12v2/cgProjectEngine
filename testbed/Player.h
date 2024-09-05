#pragma once

#include "Core/Event.h"
#include "Core/Random.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Render/Renderer.h"
#include "Resource/Rendering/cgeMesh.h"
#include "irrKlang/ik_ISound.h"
#include "irrKlang/ik_ISoundSource.h"

#include <array>
#include <cassert>
#include <glm/ext/vector_uint2.hpp>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace cge
{

inline U32_t constexpr pieceSize             = 100;
inline F32_t constexpr coinPositionIncrement = static_cast<F32_t>(pieceSize * 5);

class ScrollingTerrain
{
  private:
    static U32_t constexpr numPieces        = 10;
    static U32_t constexpr maxDestructables = 4;
    static U32_t constexpr maxObstacles     = 4;
    static U32_t constexpr maxPieces        = 4;

  public:
    using ObstacleList = std::array<Sid_t, numPieces>;
    using PieceList    = std::array<Sid_t, numPieces>;
    using PowerupList  = std::array<Sid_t, numPieces>;
    using CoinMap      = std::pmr::unordered_map<U32_t, Sid_t>;
    struct InitData
    {
        std::span<Sid_t> pieces;
        std::span<Sid_t> obstacles;
        std::span<Sid_t> destructables;
        Sid_t            magnetPowerUp;
        Sid_t            coin;
        Sid_t            speed;
    };

  public:
    void init(InitData const &initData);
    void updateTilesFromPosition(glm::vec3 position);
    B8_t handleShoot(Ray const &ray);

    ObstacleList const &getObstacles() const;
    ObstacleList const &getDestructables() const;
    PowerupList const  &getPowerUps() const;
    CoinMap const      &getCoinMap() const;
    B8_t                shouldCheckForPowerUps() const;
    void                removeCoin(CoinMap::iterator const &it);
    void                removeCoin(CoinMap::const_iterator const &it);
    void                powerUpAcquired(U32_t index);
    U32_t               removeAllCoins();

  private:
    Sid_t selectRandomPiece() const;
    Sid_t selectRandomObstacle() const;
    Sid_t selectRandomDestructable() const;

    void      addPropOfType(U32_t type, glm::mat4 const &pieceTransform);
    void      addCoins(F32_t);
    void      removeCoins(F32_t);
    void      addPowerUp(glm::mat4 const &pieceTransform);
    glm::mat4 propDisplacementTransformFromOldPiece(glm::mat4 const &pieceTransform) const;
    F32_t     randomLaneOffset() const;

    template<std::integral auto N>
    static constexpr Sid_t selectRandomFromList(std::array<Sid_t, N> list, U32_t effectiveSize)
    {
        auto  rnd = g_random.next<U32_t>(0, effectiveSize - 1);
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
    PowerupList  m_powerUps{ nullSid };

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
    Sid_t                               m_speedPowerUp{ nullSid };

    U32_t m_pieceSetSize{ 0 };
    U32_t m_obstacleSetSize{ 0 };
    U32_t m_destructableSetSize{ 0 };

    B8_t m_shouldCheckPowerUp{ true };
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
    void onTick(F32_t deltaTime);
    void onSpeedAcquired();

    [[nodiscard]] AABB      boundingBox() const;
    [[nodiscard]] glm::mat4 viewTransform() const;
    [[nodiscard]] Camera_t  getCamera() const;
    [[nodiscard]] glm::vec3 getCentroid() const;
    [[nodiscard]] F32_t     getVelocity() const;

    // TODO remove
    [[nodiscard]] glm::vec3 lastDisplacement() const;

    bool                intersectPlayerWith(ScrollingTerrain &terrain);
    void                incrementScore(U32_t increment, U32_t numCoins = 1);
    [[nodiscard]] U64_t getCurrentScore() const;

  private:
    static U8_t constexpr LANE_LEFT   = 1u << 2; // 0000'0100
    static U8_t constexpr LANE_CENTER = 1u << 1; // 0000'0010
    static U8_t constexpr LANE_RIGHT  = 1u << 0; // 0000'0001
    static inline glm::vec3 const meshCameraOffset{ 0.f, -2.f, -10.f };

  private:
    glm::vec3 displacementTick(F32_t deltaTime);

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
    F32_t     m_velocityIncrement{ 1.f };
    glm::vec3 m_lastDisplacement{};

    // event data
    union U
    {
        constexpr U()
        {
        }
        U &operator=(const U &);
        struct S
        {
            EventDataSidPair keyListener;
            EventDataSidPair speedAcquiredListener;
        };
        S                               s;
        std::array<EventDataSidPair, 2> arr;
        static_assert(std::is_standard_layout_v<S> && sizeof(S) == sizeof(decltype(arr)), "implementation failed");
    };
    U     m_listeners{};
    F32_t m_invincibilityTimer{ 0 };
    B8_t  m_invincible{ false };

    // sound data
    irrklang::ISoundSource *m_swishSoundSource{ nullptr };
    irrklang::ISound       *m_swishSound{ nullptr };

    irrklang::ISoundSource *m_bgmSource{ nullptr };
    irrklang::ISound       *m_bgm{ nullptr };

    irrklang::ISoundSource *m_invincibleMusicSource{ nullptr };
    irrklang::ISound       *m_invincibleMusic{ nullptr };
    void                    stopInvincibleMusic();
    void                    resumeNormalMusic();
    void                    stopSwishSound();
};

} // namespace cge
