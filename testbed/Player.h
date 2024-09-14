#pragma once

#include "ConstantsAndStructs.h"
#include "Core/Event.h"
#include "Core/Random.h"
#include "Core/StringUtils.h"
#include "Core/TimeUtils.h"
#include "Core/Type.h"
#include "Ornithopter.h"
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
    static U32_t constexpr numPieces                  = 10;
    static U32_t constexpr maxDestructables           = 4;
    static U32_t constexpr maxObstacles               = 4;
    static U32_t constexpr maxPieces                  = 4;
    static F32_t constexpr timeToMaxDifficultySeconds = 120.f;

    static U32_t constexpr finalEmptyPieceProbability   = 27;
    static U32_t constexpr initialEmptyPieceProbability = 32;
    static_assert(finalEmptyPieceProbability < initialEmptyPieceProbability);

    static U32_t constexpr finalObstacleProbability   = 69;
    static U32_t constexpr initialObstacleProbability = 62;
    static_assert(finalObstacleProbability > initialObstacleProbability);

    static U32_t constexpr finalMalusProbability   = 3;
    static U32_t constexpr initialMalusProbability = 5;
    static_assert(finalMalusProbability < initialMalusProbability);

    static U32_t constexpr finalPowerUpProbability   = 1;
    static U32_t constexpr initialPowerUpProbability = 2;
    static_assert(finalPowerUpProbability < initialPowerUpProbability);

  public:
    using ObstacleList  = std::array<Sid_t, numPieces>;
    using PieceList     = std::array<Sid_t, numPieces>;
    using PowerupList   = std::array<Sid_t, numPieces>;
    using PowerdownList = std::array<Sid_t, numPieces>;
    using CoinMap       = std::pmr::unordered_map<U32_t, Sid_t>;
    struct InitData
    {
        std::span<Sid_t> pieces;
        std::span<Sid_t> obstacles;
        std::span<Sid_t> destructables;
        Sid_t            magnetPowerUp;
        Sid_t            coin;
        Sid_t            speed;
        Sid_t            down;
    };

  public:
    void init(InitData const &initData);
    void updateTilesFromPosition(glm::vec3 position);
#if 0
    B8_t handleShoot(Ray const &ray);
#else
    B8_t handleShoot(AABB const &playerBox);
#endif

    void                 onTick(U64_t deltaTime);
    ObstacleList const  &getObstacles() const;
    ObstacleList const  &getDestructables() const;
    PowerupList const   &getPowerUps() const;
    PowerdownList const &getPowerDowns() const;
    CoinMap const       &getCoinMap() const;
    B8_t                 shouldCheckForPowerUps() const;
    void                 removeCoin(CoinMap::iterator const &it);
    void                 removeCoin(CoinMap::const_iterator const &it);
    void                 powerUpAcquired(U32_t index);
    void                 powerDownAcquired(U32_t index);
    U32_t                removeAllCoins();
    void                 adjustProbabilities(U64_t deltaTime);

  private:
    Sid_t selectRandomPiece() const;
    Sid_t selectRandomObstacle() const;
    Sid_t selectRandomDestructable() const;

    void      addPropOfType(U32_t type, glm::mat4 const &pieceTransform);
    void      addCoins(F32_t);
    void      removeCoins(F32_t);
    void      addPowerUp(glm::mat4 const &pieceTransform);
    void      addPowerDown(glm::mat4 const &pieceTransform);
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
    PieceList     m_pieces{ nullSid };
    ObstacleList  m_obstacles{ nullSid };
    ObstacleList  m_destructables{ nullSid };
    PowerupList   m_powerUps{ nullSid };
    PowerdownList m_powerDowns{ nullSid };

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
    Sid_t                               m_powerDown{ nullSid };

    U64_t m_elapsedTime{ 0 };
    U64_t m_timeToMaxDifficulty{ static_cast<U64_t>(timeToMaxDifficultySeconds * timeUnit64) };
    U32_t m_totalProbability{ 100 };

    F64_t m_emptyPieceProbability{ initialEmptyPieceProbability };
    F64_t m_obstacleProbability{ initialObstacleProbability };
    F64_t m_malusProbability{ initialMalusProbability };
    F64_t m_powerUpProbability{ initialPowerUpProbability };
    B8_t  m_finalProbabilityReached = false;

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
    void spawn(Camera_t const &view, EDifficulty difficulty);

    void onKey(I32_t key, I32_t action);
    void onTick(U64_t deltaTime);
    void onSpeedAcquired();
    void onPowerDown();

    [[nodiscard]] AABB      boundingBox() const;
    [[nodiscard]] glm::mat4 viewTransform() const;
    [[nodiscard]] Camera_t  getCamera() const;
    [[nodiscard]] glm::vec3 getCentroid() const;
    [[nodiscard]] F32_t     getVelocity() const;
    [[nodiscard]] F32_t     getStartVelocity() const;
    [[nodiscard]] F32_t     getMaxVelocity() const;

    // TODO remove
    [[nodiscard]] glm::vec3 lastDisplacement() const;

    bool                intersectPlayerWith(ScrollingTerrain &terrain);
    void                incrementScore(U32_t increment, U32_t numCoins = 1);
    [[nodiscard]] U64_t getCurrentScore() const;
    F32_t               remainingInvincibleTime() const;
    F32_t               remainingMalusTime() const;
    void                kill();

  private:
    static U8_t constexpr LANE_LEFT   = 1u << 2; // 0000'0100
    static U8_t constexpr LANE_CENTER = 1u << 1; // 0000'0010
    static U8_t constexpr LANE_RIGHT  = 1u << 0; // 0000'0001

  private:
    glm::vec3 displacementTick(F32_t deltaTime);
    void      stopInvincibleMusic();
    void      resumeNormalMusic();

  private:
    // main components
    union U0
    {
        U0()
        {
        }
        ~U0()
        {
        }
        Ornithopter ornithopter;
    };
    U0       m_delayedCtor;
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
    F32_t     m_difficultyMultiplier = 1.f;

    // event data
    union U
    {
        constexpr U()
        {
        }
        struct S
        {
            EventDataSidPair keyListener;
            EventDataSidPair speedAcquiredListener;
            EventDataSidPair downAcquiredListener;
        };
        S                               s;
        std::array<EventDataSidPair, 3> arr;
        static_assert(std::is_standard_layout_v<S> && sizeof(S) == sizeof(decltype(arr)), "implementation failed");
    };
    U     m_listeners{};
    F32_t m_invincibilityTimer{ -1.f };
    B8_t  m_invincible{ false };
    B8_t  m_ornithopterAlive = false;

    // sound data
    irrklang::ISoundSource *m_bgmSource{ nullptr };
    irrklang::ISound       *m_bgm{ nullptr };

    irrklang::ISoundSource *m_invincibleMusicSource{ nullptr };
    irrklang::ISound       *m_invincibleMusic{ nullptr };
};

} // namespace cge
