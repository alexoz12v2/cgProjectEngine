#include "Player.h"

#include "ConstantsAndStructs.h"
#include "Core/Event.h"
#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Core/StringUtils.h"
#include "Core/TimeUtils.h"
#include "Core/Type.h"
#include "Render/Renderer.h"
#include "Render/Renderer2d.h"
#include "Resource/HandleTable.h"
#include "Resource/Rendering/cgeMesh.h"
#include "SoundEngine.h"
#include "Utils.h"

#include <glm/common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <iterator>

namespace cge
{

inline U32_t constexpr halfPieceSize = 50;
inline U32_t constexpr numLanes      = 3;
inline F32_t constexpr laneShift     = (F32_t)pieceSize / numLanes;

// player constants
inline F32_t constexpr scoreMultiplier   = 0.1f;
inline F32_t constexpr baseShiftDelay    = 0.01f; // between 0 and 1
inline F32_t constexpr baseVelocity      = 200.f;
inline F32_t constexpr maxBaseVelocity   = 800.f;
inline F32_t constexpr invincibilityTime = 7.f;
inline F32_t constexpr speedBoost        = 2.f;
inline glm::vec3 const meshCameraOffset{ 0.f, 20.f, -9.f };
inline F32_t constexpr maxLeanRotation = 15.f * glm::pi<F32_t>() / 180.f;
inline glm::vec3 const xAxis{ 1.f, 0.f, 0.f };

// scrolling terrain constants
inline U32_t constexpr numPowerUpTypes    = std::array{ "speed", "magnet" }.size();
inline U32_t constexpr maxNumSpawnedCoins = 20;

Player::~Player()
{
    if (m_init)
    {
        for (auto const &pair : m_listeners.arr)
        { //
            g_eventQueue.removeListener(pair);
        }

        if (m_invincibleMusic)
        {
            m_invincibleMusic->stop();
            m_invincibleMusic->drop();
        }
        if (m_bgm)
        {
            m_bgm->stop();
            m_bgm->drop();
        }

        g_soundEngine()->removeSoundSource(m_bgmSource);
        g_soundEngine()->removeSoundSource(m_invincibleMusicSource);
        std::destroy_at(&m_delayedCtor.ornithopter);
    }
}

void Player::spawn(const Camera_t &view)
{
    EventArg_t listenerData{};
    listenerData.idata.p      = reinterpret_cast<Byte_t *>(this);
    m_listeners.s.keyListener = g_eventQueue.addListener(evKeyPressed, KeyCallback<Player>, listenerData);
    m_listeners.s.speedAcquiredListener =
      g_eventQueue.addListener(evSpeedAcquired, speedAcquiredCallback<Player>, listenerData);
    m_listeners.s.downAcquiredListener =
      g_eventQueue.addListener(evDownAcquired, powerDownCallback<Player>, listenerData);

    OrnithopterSpec const spec{
        .body        = CGE_SID("Body"),
        .wingUpR     = CGE_SID("wing_up.R"),
        .wingUpL     = CGE_SID("wing_up.L"),
        .wingBottomR = CGE_SID("wing_bottom.R"),
        .wingBottomL = CGE_SID("wing_bottom.L"),
    };
    std::construct_at(&m_delayedCtor.ornithopter, spec);

    m_camera = view;
    m_delayedCtor.ornithopter.init(glm::translate(glm::inverse(m_camera.viewTransform()), meshCameraOffset));

    m_invincibleMusicSource = g_soundEngine()->addSoundSourceFromFile("../assets/invincible.mp3");

    m_bgmSource        = g_soundEngine()->addSoundSourceFromFile("../assets/bgm0.mp3");
    m_bgm              = g_soundEngine()->play2D(m_bgmSource, true);
    m_init             = true;
    m_ornithopterAlive = true;

    assert(m_invincibleMusicSource && m_bgmSource);
}

static glm::mat4 computeCameraTransform(Camera_t const &camera)
{
    return glm::translate(glm::mat4(1.0f), camera.position);
}

void Player::onTick(U64_t deltaTimeI)
{
    if (!m_ornithopterAlive)
    {
        return;
    }

    F32_t deltaTimeF = static_cast<F32_t>(deltaTimeI) / timeUnit64;
    if (m_invincible)
    {
        m_invincibilityTimer += deltaTimeF;
        printf("[Player] INVINCIBLE, Timer = %f\n", m_invincibilityTimer);
        if (m_invincibilityTimer > invincibilityTime)
        {
            m_invincibilityTimer = -1.f;
            stopInvincibleMusic();
            resumeNormalMusic();
            m_invincible = false;
        }
    }
    if (m_invincibilityTimer >= 0.f)
    {
        m_invincibilityTimer += deltaTimeF;
        if (m_invincibilityTimer > invincibilityTime)
        {
            m_invincibilityTimer = -1.f;
        }
    }

    if (m_intersected)
    {
        EventArg_t evData{};
        evData.idata.u64 = m_score;
        g_eventQueue.emit(evGameOver, evData);
    }
    else
    {
        // compute forward movement
        glm::vec3 const displacement = displacementTick(deltaTimeF);
        m_score += glm::max(static_cast<decltype(1ULL)>(displacement.y * scoreMultiplier), 1ULL);
        m_oldPosition = m_camera.position;

        // compute side movement
        F32_t disp = m_oldPosition.x;
        if (glm::abs(m_camera.position.x - m_targetXPos) > std::numeric_limits<F32_t>::epsilon())
        {
            disp = glm::mix(m_camera.position.x, m_targetXPos, 1.f - glm::pow(baseShiftDelay, deltaTimeF));

            if (glm::abs(disp - m_targetXPos) <= 0.5f)
            {
                disp = m_targetXPos;
                m_delayedCtor.ornithopter.stopSwish();
            }
        }

        // first apply old position to mesh
        F32_t     interpolant    = 0.01f; //(getVelocity() - baseVelocity) / (maxBaseVelocity - baseVelocity);
        F32_t     leanAngle      = glm::mix(0.f, maxLeanRotation, interpolant);
        glm::vec3 cameraDistance = meshCameraOffset;
        cameraDistance.y         = glm::mix(meshCameraOffset.y, meshCameraOffset.y + 5.f, interpolant);
        glm::mat4 t              = glm::rotate(glm::mat4(1.f), leanAngle, xAxis);
        glm::mat4 tr             = glm::translate(glm::mat4(1.f), cameraDistance);
        m_delayedCtor.ornithopter.onTick(
          deltaTimeI,
          { .playerTransform = t, .playerTranslate = tr, .cameraTransform = computeCameraTransform(m_camera) });

        // then update position
        m_camera.position.x = disp;
        m_camera.position += displacement;
        F32_t x = glm::log(static_cast<F32_t>(m_score));
        x *= x;
        m_velocityIncrement = glm::max(glm::abs(x), 1.f);
#if 0
        printf("[Player] New Displacement after deltaTime %f <=> %f\n", deltaTimeF, displacement.y);
#endif
    }
}
void Player::resumeNormalMusic()
{
    if (m_bgm)
    {
        m_bgm->setIsPaused(false);
    }
    else
    {
        m_bgm = g_soundEngine()->play2D(m_bgmSource, true);
    }
}
void Player::stopInvincibleMusic()
{
    if (m_invincibleMusic)
    {
        m_invincibleMusic->stop();
        m_invincibleMusic->drop();
    }
    else
    { //
        g_soundEngine()->stopAllSoundsOfSoundSource(m_invincibleMusicSource);
    }
}

void Player::onSpeedAcquired()
{ //
    m_invincibilityTimer = 0.f;
    if (!m_invincible)
    {
        m_invincible      = true;
        m_invincibleMusic = g_soundEngine()->play2D(m_invincibleMusicSource, true);
        if (m_bgm)
        { //
            m_bgm->setIsPaused();
        }
        else
        { //
            g_soundEngine()->stopAllSoundsOfSoundSource(m_bgmSource);
        }
    }
}

glm::vec3 Player::displacementTick(F32_t deltaTime)
{
    F32_t const velocity = getVelocity();

    // Calculate the movement direction based on camera's forward vector
    glm::vec3 direction = m_camera.forward;
    if (direction != glm::vec3(0.F))
    {
        direction = glm::normalize(direction);
    }

    glm::vec3 const displacement = velocity * direction * deltaTime;

    return displacement;
}

void Player::onKey(I32_t key, I32_t action)
{
    if (!m_ornithopterAlive)
    {
        return;
    }

    if (action == action::PRESS)
    {
        switch (key)
        {
        case key::A:
            if ((m_lane & LANE_LEFT) == 0)
            {
                m_lane <<= 1;
                m_targetXPos -= laneShift;
                m_delayedCtor.ornithopter.playSwish();
            }
            break;
        case key::D:
            if ((m_lane & LANE_RIGHT) == 0)
            {
                m_lane >>= 1;
                m_targetXPos += laneShift;
                m_delayedCtor.ornithopter.playSwish();
            }
            break;
        default:
            break;
        }
    }

    if ((action == action::PRESS || action == action::REPEAT) && key == key::SPACE)
    {
        m_delayedCtor.ornithopter.playGun();
        EventArg_t evArg{};
        evArg.fdata.f32[0] = m_oldPosition[0];
        evArg.fdata.f32[1] = m_oldPosition[1];
        evArg.fdata.f32[2] = m_oldPosition[2];
        evArg.idata.i16[0] = 0;
        evArg.idata.i16[1] = 1;
        evArg.idata.i16[2] = 0;
        g_eventQueue.emit(evShoot, evArg);
    }
}

AABB Player::boundingBox() const
{
    return m_delayedCtor.ornithopter.bodyBoundingBox();
}

glm::mat4 Player::viewTransform() const
{ //
    return m_camera.viewTransform();
}

Camera_t Player::getCamera() const
{ //
    return m_camera;
}

glm::vec3 Player::lastDisplacement() const
{ //
    return m_lastDisplacement;
}

glm::vec3 Player::getCentroid() const
{
    return centroid(boundingBox());
}

void ScrollingTerrain::init(InitData const &initData)
{
    auto const initialMt =
      glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(1.f, 1.f, 2.f)), glm::vec3(0.f, 0.f, -5.f));
    static U32_t constexpr offset = 1;

    // load all available meshes
    m_pieceSetSize        = glm::min(m_pieceSet.size(), initData.pieces.size());
    m_obstacleSetSize     = glm::min(m_obstacleSet.size(), initData.obstacles.size());
    m_destructableSetSize = glm::min(m_destructableSet.size(), initData.destructables.size());
    std::copy_n(initData.pieces.begin(), m_pieceSetSize, std::begin(m_pieceSet));
    std::copy_n(initData.obstacles.begin(), m_obstacleSetSize, std::begin(m_obstacleSet));
    std::copy_n(initData.destructables.begin(), m_destructableSetSize, std::begin(m_destructableSet));
    m_coin          = initData.coin;
    m_magnetPowerUp = initData.magnetPowerUp;
    m_speedPowerUp  = initData.speed;
    m_powerDown     = initData.down;

    // spawn initial platforms
    for (U32_t index = 0; index != numPieces; ++index)
    {
        F32_t const     yOff    = (static_cast<F32_t>(index) - offset) * pieceSize;
        glm::mat4 const t       = glm::translate(initialMt, glm::vec3(0.f, yOff, 0.f));
        Sid_t           meshSid = selectRandomPiece();

        m_pieces[index] = g_scene.addNode(meshSid);
        g_scene.getNodeBySid(m_pieces[index]).transform(t);
    }

    assert(m_pieceSetSize && m_obstacleSetSize && m_destructableSetSize);
    assert(m_coin != nullSid && m_magnetPowerUp != nullSid && m_speedPowerUp != nullSid);
}

void ScrollingTerrain::updateTilesFromPosition(glm::vec3 position)
{
    // check if player moved one tile forward
    U32_t const     nextPieceIdx   = (m_first + 1) % numPieces;
    glm::mat4 const pieceTransform = g_scene.getNodeBySid(m_pieces[nextPieceIdx]).getTransform();
    glm::vec4 const piecePosition  = pieceTransform[3];

    // if yes, then update first and last and translate everything from first to last
    if (position.y - piecePosition.y > static_cast<F32_t>(pieceSize >> 1))
    {
        m_shouldCheckPowerUp = true;
        glm::vec3 displacement{ 0.f, pieceSize * numPieces, 0.f };
        g_scene.getNodeBySid(m_pieces[m_first]).transform(glm::translate(glm::mat4(1.f), displacement));

        // remove the obstacle (if any) from the moved piece
        if (m_obstacles[m_first] != nullSid)
        { // remove from scene
            g_scene.removeNode(m_obstacles[m_first]);
            m_obstacles[m_first] = nullSid;
        }

        // remove the powerup (if any) from the moved piece
        if (m_powerUps[m_first] != nullSid)
        {
            g_scene.removeNode(m_powerUps[m_first]);
            m_powerUps[m_first] = nullSid;
        }

        // remove powerdown (if any) from the moved piece
        if (m_powerDowns[m_first] != nullSid)
        {
            g_scene.removeNode(m_powerDowns[m_first]);
            m_powerDowns[m_first] = nullSid;
        }

        // add new obstacles or powerup in the moved piece
        // when at max difficulty:
        // take a number from 0 to 100. 0 - 27 -> empty, 28 - 94 -> obstacle, 95 - 98 -> down, 99 - 100 -> powerup
        U32_t threshold0 = m_emptyPieceProbability;
        U32_t threshold1 = m_obstacleProbability + threshold0;
        U32_t threshold2 = m_malusProbability + threshold1;

        U32_t const num = g_random.next<U32_t>(0, m_totalProbability);
        if (num > threshold0)
        {
            if (num < threshold1)
            {
                addPropOfType(g_random.next<U32_t>(0, 1), pieceTransform);
            }
            else if (num < threshold2)
            {
                addPowerDown(pieceTransform);
            }
            else
            {
                addPowerUp(pieceTransform);
            }
        }

        // maintain indices
        m_first = nextPieceIdx;

        // remove old coins
        removeCoins(position.y);

        // possibly add coins
        if (m_coinMap.size() < maxNumSpawnedCoins)
        {
            addCoins(m_coinYCoord);
            m_coinYCoord += coinPositionIncrement;
        }
    }
}

static constexpr B8_t isOverlapping2D(AABB const &box1, AABB const &box2)
{
    return box1.mm.max.x >= box2.mm.min.x && box1.mm.min.x <= box2.mm.max.x && box1.mm.max.z >= box2.mm.min.z
           && box1.mm.min.z <= box2.mm.max.z;
}

B8_t ScrollingTerrain::handleShoot(AABB const &playerBox)
{
    B8_t  foundDestroyable    = false;
    F32_t minDistance         = std::numeric_limits<F32_t>::max();
    Sid_t closestDestructible = nullSid;

    // check all destroyable obstacles
    for (Sid_t const &sid : m_destructables)
    {
        if (sid == nullSid)
        {
            continue;
        }

        SceneNode_s const &node = g_scene.getNodeBySid(sid);
        AABB const        &box  = globalSpaceBB(node, g_handleTable.getMesh(node.getSid()).box);
        if (isOverlapping2D(playerBox, box))
        {
            glm::vec2 playerCenter2D =
              (glm::vec2(playerBox.mm.min.x, playerBox.mm.min.y) + glm::vec2(playerBox.mm.max.x, playerBox.mm.max.y))
              * 0.5f;
            glm::vec2 boxCenter2D =
              (glm::vec2(box.mm.min.x, box.mm.min.y) + glm::vec2(box.mm.max.x, box.mm.max.y)) * 0.5f;
            F32_t distance = glm::length(playerCenter2D - boxCenter2D);
            assert(distance > 0);
            if (distance < minDistance)
            {
                minDistance         = distance;
                foundDestroyable    = true;
                closestDestructible = sid;
            }
        }
    }
    // check all non-destroyable obstacles
    for (Sid_t const &sid : m_obstacles)
    {
        if (sid == nullSid)
        {
            continue;
        }

        SceneNode_s const &node = g_scene.getNodeBySid(sid);
        AABB const        &box  = globalSpaceBB(node, g_handleTable.getMesh(node.getSid()).box);
        if (isOverlapping2D(playerBox, box))
        {
            glm::vec2 playerCenter2D =
              (glm::vec2(playerBox.mm.min.x, playerBox.mm.min.y) + glm::vec2(playerBox.mm.max.x, playerBox.mm.max.y))
              * 0.5f;
            glm::vec2 boxCenter2D =
              (glm::vec2(box.mm.min.x, box.mm.min.y) + glm::vec2(box.mm.max.x, box.mm.max.y)) * 0.5f;
            F32_t distance = glm::length(playerCenter2D - boxCenter2D);
            assert(distance > 0);
            if (distance < minDistance)
            {
                foundDestroyable = false;
                break;
            }
        }
    }

    if (foundDestroyable)
    {
        printf("[ScrollingTerrain] destructing one piece\n");
        auto it = std::find(m_destructables.begin(), m_destructables.end(), closestDestructible);
        if (it != m_destructables.end())
        {
            g_scene.removeNode(*it);
            *it = nullSid; // Set to nullSid
        }
    }

    return foundDestroyable;
}

ScrollingTerrain::ObstacleList const &ScrollingTerrain::getObstacles() const
{ // getter
    return m_obstacles;
}

ScrollingTerrain::ObstacleList const &ScrollingTerrain::getDestructables() const
{ // getter
    return m_destructables;
}

ScrollingTerrain::PowerupList const &ScrollingTerrain::getPowerUps() const
{ // getter
    return m_powerUps;
}

std::pmr::unordered_map<U32_t, Sid_t> const &ScrollingTerrain::getCoinMap() const
{ // getter
    return m_coinMap;
}

B8_t ScrollingTerrain::shouldCheckForPowerUps() const
{ // getter
    return m_shouldCheckPowerUp;
}

void ScrollingTerrain::removeCoin(CoinMap::iterator const &it)
{
    g_scene.removeNode(it->second);
    m_coinMap.erase(it);
}

void ScrollingTerrain::removeCoin(CoinMap::const_iterator const &it)
{
    g_scene.removeNode(it->second);
    m_coinMap.erase(it);
}

void ScrollingTerrain::powerUpAcquired(U32_t index)
{
    assert(index < m_powerUps.size() && m_powerUps[index] != nullSid);
    EventArg_t evData{};
    Sid_t      sid = m_powerUps[index];

    // make sure that check for powerups is made once when one is acquired
    m_shouldCheckPowerUp = false;

    // clean up
    g_scene.removeNode(m_powerUps[index]);
    m_powerUps[index] = nullSid;

    // emit event based on the type of power up
    if (sid == m_magnetPowerUp)
    {
        g_eventQueue.emit(evMagnetAcquired, evData);
    }
    else if (sid == m_speedPowerUp)
    {
        g_eventQueue.emit(evSpeedAcquired, evData);
    }
}

U32_t ScrollingTerrain::removeAllCoins()
{ //
    U32_t size = m_coinMap.size();
    for (auto const &[y, sid] : m_coinMap)
    { //
        assert(sid != nullSid);
        g_scene.removeNode(sid);
    }
    m_coinMap.clear();
    return size;
}

Sid_t ScrollingTerrain::selectRandomPiece() const
{ //
    return selectRandomFromList(m_pieceSet, m_pieceSetSize);
}

Sid_t ScrollingTerrain::selectRandomObstacle() const
{ //
    return selectRandomFromList(m_obstacleSet, m_obstacleSetSize);
}

Sid_t ScrollingTerrain::selectRandomDestructable() const
{ //
    return selectRandomFromList(m_destructableSet, m_destructableSetSize);
}

void ScrollingTerrain::addPropOfType(U32_t type, glm::mat4 const &pieceTransform)
{
    glm::mat4 const t{ propDisplacementTransformFromOldPiece(pieceTransform) };
    if (type == 0)
    { // choose an obstacle and spawn it
        U32_t const obstacleIdx = g_random.next<U32_t>(0, m_obstacleSetSize - 1);
        Sid_t const sid         = m_obstacleSet[obstacleIdx];
        m_obstacles[m_first]    = g_scene.addNode(sid);
        g_scene.getNodeBySid(m_obstacles[m_first]).transform(t * glm::scale(glm::mat4{ 1.f }, glm::vec3(9.f)));
    }
    else if (type == 1)
    { // choose a destructable and spawn it
        U32_t const destructableIdx = g_random.next<U32_t>(0, m_destructableSetSize - 1);
        Sid_t const sid             = m_destructableSet[destructableIdx];
        m_destructables[m_first]    = g_scene.addNode(sid);
        g_scene.getNodeBySid(m_destructables[m_first]).transform(t);
    }
}

void ScrollingTerrain::addCoins(F32_t pieceYCoord)
{
    static F32_t constexpr coinHeight{ 50.f };
    static U32_t constexpr maxNumCoinsPerTile{ 5 };
    static U32_t constexpr threshold{ 3 };
    static F32_t constexpr coinShift{ laneShift * 1.2f };
    static F32_t constexpr heightShift{ 4.f };
    static F32_t constexpr betweenDistance{ 5.f };
    U32_t         numSpawnedCoins{ 0 };
    F32_t         lastPos   = pieceYCoord;
    Mesh_s const &coinMesh  = g_handleTable.getMesh(m_coin);
    F32_t const   coinDepth = coinMesh.box.mm.max.y - coinMesh.box.mm.min.y;
    F32_t const   increment = coinDepth + betweenDistance;

    while (g_random.next<U32_t>(0, maxNumCoinsPerTile - numSpawnedCoins) < threshold)
    {
        Sid_t coinSceneSid = g_scene.addNode(m_coin);
        F32_t xCoord       = g_random.next<F32_t>() * coinShift;
        F32_t zCoord       = (g_random.next<F32_t>() - 0.5f) * heightShift + coinHeight;

        g_scene.getNodeBySid(coinSceneSid)
          .transform(glm::translate(glm::mat4(1.f), glm::vec3(xCoord, lastPos, zCoord)));
        m_coinMap.try_emplace(static_cast<U32_t>(lastPos), coinSceneSid);

        lastPos += increment;
        ++numSpawnedCoins;
    }
}

void ScrollingTerrain::removeCoins(F32_t threshold)
{
    std::erase_if(
      m_coinMap,
      [threshold](
        const auto &positionSidPair) { // if the coin is in the y range of the piece begin removed, then remove it
          if (positionSidPair.first <= threshold)
          {
              Sid_t sceneSid = positionSidPair.second;
              g_scene.removeNode(sceneSid);
              return true;
          }
          return false;
      });
}

void ScrollingTerrain::addPowerUp(glm::mat4 const &pieceTransform)
{
    glm::mat4 const t{ propDisplacementTransformFromOldPiece(pieceTransform) };
    switch (U32_t const powerUpType = g_random.next<U32_t>(0, numPowerUpTypes - 1))
    {
    case 0: // explosive magnet
        m_powerUps[m_first] = g_scene.addNode(m_magnetPowerUp);
        // fix orientation
        g_scene.getNodeBySid(m_powerUps[m_first])
          .transform(glm::rotate(glm::mat4(1), glm::pi<F32_t>(), glm::vec3(0.f, 0.f, 1.f)));
        break;
    case 1: // invincibility rocket
        m_powerUps[m_first] = g_scene.addNode(m_speedPowerUp);
        break;
    default:
        assert(false);
        break;
    }

    g_scene.getNodeBySid(m_powerUps[m_first]).transform(t);
}

glm::mat4 ScrollingTerrain::propDisplacementTransformFromOldPiece(glm::mat4 const &pieceTransform) const
{
    F32_t x = randomLaneOffset();
    return glm::translate(pieceTransform, glm::vec3(x, pieceSize * (numPieces - 1), 0.f));
}

F32_t ScrollingTerrain::randomLaneOffset() const
{
    std::array<F32_t, 3> arr = { -laneShift, 0, laneShift };
    std::ranges::shuffle(arr, g_random.getGen());
    return arr[0];
}
void ScrollingTerrain::onTick(U64_t deltaTime)
{
    static F32_t constexpr maxDisplacement  = 0.01f;
    static F32_t constexpr frequency        = 5.f * glm::pi<F32_t>() * 0.5f;
    static F32_t constexpr radiansPerSecond = 120.f * glm::pi<F32_t>() / 180.f;
    m_elapsedTime += deltaTime;

    // rotate by a bit all coins
    for (auto const &[pos, coin] : m_coinMap)
    {
        if (coin == nullSid)
        {
            continue;
        }
        auto     &node = g_scene.getNodeBySid(coin);
        glm::mat4 p    = node.getTransform();
        g_scene.getNodeBySid(coin).setTransform(
          p * glm::rotate(glm::mat4(1.f), deltaTime * radiansPerSecond / timeUnit64, glm::vec3(0.f, 0.f, 1.f)));
    }

    // move up and down by a bit all power-ups
    for (Sid_t const &sid : m_powerUps)
    {
        if (sid == nullSid)
        {
            continue;
        }
        auto &node = g_scene.getNodeBySid(sid);
        auto  t    = node.getTransform();
        F32_t disp = maxDisplacement * glm::sin(frequency * m_elapsedTime / timeUnit64);
        t *= glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, disp));
        while (t[3].z > 1.f)
        {
            t[3].z += 0.1f;
        }
        node.setTransform(t);
    }

    for (Sid_t const &sid : m_powerDowns)
    {
        if (sid == nullSid)
        {
            continue;
        }
        auto &node = g_scene.getNodeBySid(sid);
        auto  t    = node.getTransform();
        F32_t disp = maxDisplacement * glm::sin(frequency * m_elapsedTime / timeUnit64);
        t *= glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, disp));
        while (t[3].z > 1.f)
        {
            t[3].z += 0.1f;
        }
        node.setTransform(t);
    }

    adjustProbabilities(deltaTime);
}
void ScrollingTerrain::addPowerDown(const glm::mat4 &pieceTransform)
{
    glm::mat4 const t{ propDisplacementTransformFromOldPiece(pieceTransform) };
    m_powerDowns[m_first] = g_scene.addNode(m_powerDown);
    g_scene.getNodeBySid(m_powerDowns[m_first]).transform(t);
}

ScrollingTerrain::PowerdownList const &ScrollingTerrain::getPowerDowns() const
{
    return m_powerDowns;
}

void ScrollingTerrain::powerDownAcquired(U32_t index)
{
    // make sure that check for powerup or down is made once when one is acquired (refreshed on tile refresh)
    m_shouldCheckPowerUp = false;
    g_scene.removeNode(m_powerDowns[index]);
    m_powerDowns[index] = nullSid;
    EventArg_t eventArg{};
    g_eventQueue.emit(evDownAcquired, eventArg);
}

void ScrollingTerrain::adjustProbabilities(U64_t deltaTime)
{
    F64_t deltaTimeF = static_cast<F64_t>(deltaTime) / timeUnit64;
    printf(
      "[ScrollingTerrain] emptyPiece: %f, obstacle: %f, malus: %f, powerup: %f\n",
      m_emptyPieceProbability,
      m_obstacleProbability,
      m_malusProbability,
      m_powerUpProbability);
    // Calculate the interpolation factor
    F64_t t = std::min(1.0, deltaTimeF / m_timeToMaxDifficulty);

    if (!m_finalProbabilityReached)
    {
        // Interpolating probabilities
        B8_t nothingChanged = true;
        if (m_emptyPieceProbability >= finalEmptyPieceProbability)
        {
            m_emptyPieceProbability -= t * (initialEmptyPieceProbability - finalEmptyPieceProbability);
            nothingChanged = false;
        }
        else
        {
            m_emptyPieceProbability = finalEmptyPieceProbability;
        }

        if (m_obstacleProbability <= finalObstacleProbability)
        {
            m_obstacleProbability += t * (finalObstacleProbability - initialObstacleProbability);
            nothingChanged = false;
        }
        else
        {
            m_obstacleProbability = finalObstacleProbability;
        }

        if (m_malusProbability >= finalMalusProbability)
        {
            m_malusProbability -= t * (initialMalusProbability - finalMalusProbability);
            nothingChanged = false;
        }
        else
        {
            m_malusProbability = finalMalusProbability;
        }

        if (m_powerUpProbability >= finalPowerUpProbability)
        {
            m_powerUpProbability -= t * (initialPowerUpProbability - finalPowerUpProbability);
            nothingChanged = false;
        }
        else
        {
            m_powerUpProbability = finalPowerUpProbability;
        }

        if (nothingChanged)
        {
            m_finalProbabilityReached = true;
        }

        // Calculate the total probability
        m_totalProbability =
          glm::ceil(m_emptyPieceProbability + m_obstacleProbability + m_malusProbability + m_powerUpProbability);
    }
}

bool Player::intersectPlayerWith(ScrollingTerrain &terrain)
{
    if (!m_ornithopterAlive)
    {
        return false;
    }
    AABB const      playerBox{ boundingBox() };
    glm::vec3 const newPosition{ centroid(playerBox) };
    Ray const       playerRay{ m_oldPosition, newPosition - m_oldPosition };
    F32_t const     movementDistance = newPosition.y - m_oldPosition.y;

    m_intersected = false;
    if (!m_invincible)
    {
        for (Sid_t const &pObs : terrain.getObstacles())
        {
            HandleTable_s::Ref_s ref{ nullRef };
            Sid_t                meshSid{ nullSid };
            if (pObs != nullSid && (ref = g_handleTable.get(meshSid = g_scene.getNodeBySid(pObs).getSid())).hasValue())
            {
                auto const &mesh   = ref.asMesh();
                auto const &box    = mesh.box;
                auto const  obsBox = globalSpaceBB(g_scene.getNodeBySid(pObs), box);
                Hit_t const res    = intersect(playerRay, obsBox);
                if (res.isect && (res.t <= movementDistance && res.p.y > m_oldPosition.y))
                {
                    m_intersected = true;
                    printf("\033[31m[PLAYER] INTERSECTIONSIONSIDOFNSDIOFJ\033[0m\n");
                    return m_intersected;
                }
            }
        }

        for (Sid_t const &pObs : terrain.getDestructables())
        {
            HandleTable_s::Ref_s ref{ nullRef };
            Sid_t                meshSid{ nullSid };
            if (pObs != nullSid && (ref = g_handleTable.get(meshSid = g_scene.getNodeBySid(pObs).getSid())).hasValue())
            {
                auto const &mesh   = ref.asMesh();
                auto const &box    = mesh.box;
                auto const  obsBox = globalSpaceBB(g_scene.getNodeBySid(pObs), box);
                Hit_t const res    = intersect(playerRay, obsBox);
                if (res.isect && (res.t <= movementDistance && res.p.y > m_oldPosition.y))
                {
                    m_intersected = true;
                    return m_intersected;
                }
            }
        }
    }

    if (terrain.shouldCheckForPowerUps())
    {
        U32_t index = 0;
        for (Sid_t const &sceneSid : terrain.getPowerUps())
        {
            if (sceneSid != nullSid)
            {
                Mesh_s const &mesh = g_handleTable.getMesh(g_scene.getNodeBySid(sceneSid).getSid());
                AABB const   &box  = globalSpaceBB(g_scene.getNodeBySid(sceneSid), mesh.box);
                Hit_t const   res  = intersect(playerRay, box);
                if (res.isect && (res.t <= movementDistance && res.p.y > m_oldPosition.y))
                {
                    terrain.powerUpAcquired(index);
                    printf("[Player] POWER UP UP UP\n");
                    return false;
                }
            }
            ++index;
        }

        index = 0;
        for (Sid_t const &sceneSid : terrain.getPowerDowns())
        {
            if (sceneSid != nullSid)
            {
                Mesh_s const &mesh = g_handleTable.getMesh(g_scene.getNodeBySid(sceneSid).getSid());
                AABB const   &box  = globalSpaceBB(g_scene.getNodeBySid(sceneSid), mesh.box);
                Hit_t const   res  = intersect(playerRay, box);
                if (res.isect && (res.t <= movementDistance && res.p.y > m_oldPosition.y))
                {
                    terrain.powerDownAcquired(index);
                    printf("[Player] POWER DOWN DOWN DOWN\n");
                    return false;
                }
            }
            ++index;
        }
    }

    return m_intersected;
}

void Player::incrementScore(U32_t increment, U32_t numCoins)
{
    m_score += static_cast<U64_t>(increment) * numCoins;
}

U64_t Player::getCurrentScore() const
{
    return m_score;
}

F32_t Player::getVelocity() const
{
    B8_t cond = m_invincibilityTimer > 0 && m_invincibilityTimer < invincibilityTime;
    return (cond ? speedBoost : 1.f) * glm::min(baseVelocity + m_velocityIncrement, maxBaseVelocity);
}

F32_t Player::remainingInvincibleTime() const
{
    if (!m_invincible)
    {
        return -1.f;
    }

    return invincibilityTime - m_invincibilityTimer;
}
void Player::kill()
{
    m_delayedCtor.ornithopter.stopAllSounds();
    m_ornithopterAlive = false;
}
F32_t Player::getStartVelocity() const
{
    return baseVelocity;
}
F32_t Player::getMaxVelocity() const
{
    return maxBaseVelocity * speedBoost;
}

void Player::onPowerDown()
{
    m_invincibilityTimer = 0.f;
}
F32_t Player::remainingMalusTime() const
{
    if (!m_invincible && m_invincibilityTimer > 0.f)
    {
        return invincibilityTime - m_invincibilityTimer;
    }
    return -1.f;
}

} // namespace cge