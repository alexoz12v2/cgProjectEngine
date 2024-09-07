#include "Player.h"

#include "ConstantsAndStructs.h"
#include "Core/Event.h"
#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Core/StringUtils.h"
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
static F32_t constexpr scoreMultiplier   = 0.1f;
static F32_t constexpr baseShiftDelay    = 0.15f; // between 0 and 1
static F32_t constexpr baseVelocity      = 100.f;
static F32_t constexpr maxBaseVelocity   = 400.f;
static F32_t constexpr invincibilityTime = 7.f;
static F32_t constexpr speedBoost        = 2.f;

// scrolling terrain constants
static U32_t constexpr numPowerUpTypes    = std::array{ "speed", "magnet" }.size();
static U32_t constexpr maxNumSpawnedCoins = 20;

Player::~Player()
{
    if (m_init)
    {
        for (auto const &pair : m_listeners.arr)
        { //
            g_eventQueue.removeListener(pair);
        }

        if (m_swishSound)
        {
            m_swishSound->stop();
            m_swishSound->drop();
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
        g_soundEngine()->removeSoundSource(m_swishSoundSource);
        g_soundEngine()->removeSoundSource(m_invincibleMusicSource);
        g_scene.removeNode(m_sid);
    }
}

void Player::spawn(const Camera_t &view, Sid_t meshSid)
{
    EventArg_t listenerData{};
    listenerData.idata.p      = reinterpret_cast<Byte_t *>(this);
    m_listeners.s.keyListener = g_eventQueue.addListener(evKeyPressed, KeyCallback<Player>, listenerData);
    m_listeners.s.speedAcquiredListener =
      g_eventQueue.addListener(evSpeedAcquired, speedAcquiredCallback<Player>, listenerData);

    m_sid  = g_scene.addNode(meshSid);
    m_mesh = &g_handleTable.get(meshSid).asMesh();

    m_camera = view;

    g_scene.getNodeBySid(m_sid).setTransform(glm::translate(glm::inverse(m_camera.viewTransform()), meshCameraOffset));

    m_swishSoundSource      = g_soundEngine()->addSoundSourceFromFile("../assets/swish.mp3");
    m_invincibleMusicSource = g_soundEngine()->addSoundSourceFromFile("../assets/invincible.mp3");

    m_bgmSource = g_soundEngine()->addSoundSourceFromFile("../assets/bgm0.mp3");
    m_bgm       = g_soundEngine()->play2D(m_bgmSource, true);
    m_init      = true;

    assert(m_swishSoundSource && m_invincibleMusicSource && m_bgmSource);
}

void Player::onTick(F32_t deltaTime)
{
    if (m_invincible)
    { //
        m_invincibilityTimer += deltaTime;
        printf("[Player] INVINCIBLE, Timer = %f\n", m_invincibilityTimer);
        if (m_invincibilityTimer > invincibilityTime)
        {
            stopInvincibleMusic();
            resumeNormalMusic();
            m_invincible = false;
        }
    }

    if (m_intersected)
    { //
        EventArg_t evData{};
        evData.idata.u64 = m_score;
        g_eventQueue.emit(evGameOver, evData);
    }
    else
    {
        // compute forward movement
        glm::vec3 const displacement = displacementTick(deltaTime);
        m_score += glm::max(static_cast<decltype(1ULL)>(displacement.y * scoreMultiplier), 1ULL);
        m_oldPosition = m_camera.position;

        // compute side movement
        F32_t disp = m_oldPosition.x;
        if (glm::abs(m_camera.position.x - m_targetXPos) > std::numeric_limits<F32_t>::epsilon())
        {
            disp = glm::mix(m_camera.position.x, m_targetXPos, 1.f - glm::pow(baseShiftDelay, deltaTime));

            if (glm::abs(disp - m_targetXPos) <= 0.5f)
            {
                disp = m_targetXPos;
                stopSwishSound();
            }
        }

        // first apply old position to mesh
        g_scene.getNodeBySid(m_sid).setTransform(
          glm::translate(glm::inverse(m_camera.viewTransform()), meshCameraOffset));
        // then update position
        m_camera.position.x = disp;
        m_camera.position += displacement;
        m_velocityIncrement = glm::max(glm::abs(glm::log(static_cast<F32_t>(m_score))), 1.f);
#if 0
        printf("[Player] New Displacement after deltaTime %f <=> %f\n", deltaTime, displacement.y);
#endif
    }
}
void Player::stopSwishSound()
{
    if (m_swishSound)
    {
        m_swishSound->stop();
        m_swishSound->drop();
        m_swishSound = nullptr;
    }
}
void Player::resumeNormalMusic()
{
    if (m_bgm)
    { //
        m_bgm->setIsPaused(false);
    }
    else
    { //
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
    if (action == action::PRESS)
    {
        switch (key)
        {
        case key::A:
            if ((m_lane & LANE_LEFT) == 0)
            {
                m_lane <<= 1;
                m_targetXPos -= laneShift;
                m_swishSound = g_soundEngine()->play2D(m_swishSoundSource);
            }
            break;
        case key::D:
            if ((m_lane & LANE_RIGHT) == 0)
            {
                m_lane >>= 1;
                m_targetXPos += laneShift;
                m_swishSound = g_soundEngine()->play2D(m_swishSoundSource);
            }
            break;
        default:
            break;
        }
    }

    if ((action == action::PRESS || action == action::REPEAT) && key == key::SPACE)
    {
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
    return globalSpaceBB(m_sid, m_mesh->box);
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
{ //
    return centroid(globalSpaceBB(g_scene.getNodeBySid(m_sid), m_mesh->box));
}

void ScrollingTerrain::init(InitData const &initData)
{
    auto const identity           = glm::mat4(1.f);
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

    // spawn initial platforms
    for (U32_t index = 0; index != numPieces; ++index)
    {
        F32_t const     yOff    = (static_cast<F32_t>(index) - offset) * pieceSize;
        glm::mat4 const t       = glm::translate(identity, glm::vec3(0.f, yOff, 0.f));
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

        // add new obstacles or powerup in the moved piece
        // take a number from 0 to 100. 0 - 30 -> empty, 31 - 98 -> obstacle, 99 - 100 -> powerup
        U32_t const num = g_random.next<U32_t>(0, 100);
        if (num > 30)
        {
            if (num < 99)
            { //
                addPropOfType(g_random.next<U32_t>(0, 1), pieceTransform);
            }
            else
            { //
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

#if 0
B8_t ScrollingTerrain::handleShoot(Ray const &rayInput)
{
    Ray nRay                     = rayInput;
    nRay.orig.z                  = 0;
    float closestT               = std::numeric_limits<float>::max();
    bool  foundInDestructables   = false;
    Sid_t closestDestructableSid = nullSid;

    printf(
      "[ScrollingTerrain] Intersecting ray {\n\torigin: %f %f %f\n\tdirection: %f %f %f\n}\n",
      nRay.orig.x,
      nRay.orig.y,
      nRay.orig.z,
      nRay.dir.x,
      nRay.dir.y,
      nRay.dir.z);
    // Check destructables first
    for (auto const &dsid : m_destructables)
    {
        if (dsid == nullSid)
        {
            continue;
        }

        Sid_t const   meshSid = g_scene.getNodeBySid(dsid).getSid();
        Mesh_s const &mesh    = g_handleTable.getMesh(meshSid);
        AABB          obsBox  = globalSpaceBB(g_scene.getNodeBySid(dsid), mesh.box);
        obsBox.mm.min.z       = -10.f;
        obsBox.mm.max.z       = 10.f;

        printf(
          "[ScrollingTerrain] With AABB {\n\tmin: %f %f %f\n\tmax: %f %f %f\n}\n",
          obsBox.mm.min.x,
          obsBox.mm.min.y,
          obsBox.mm.min.z,
          obsBox.mm.max.x,
          obsBox.mm.max.y,
          obsBox.mm.max.z);
        Hit_t hit = intersect(nRay, obsBox);
        if (hit.isect && hit.t < closestT)
        {
            closestT               = hit.t;
            foundInDestructables   = true;
            closestDestructableSid = dsid;
            printf("[ScrollingTerrain] found new nearest destructible intersection at distance at %f\n", hit.t);
        }
    }
    printf("[ScrollingTerrain] Closest hit found in destructible: %s\n", (foundInDestructables ? "true" : "false"));
    printf("[ScrollingTerrain] Now checking obstacles...\n");

    // Check obstacles
    for (auto const &osid : m_obstacles)
    {
        if (osid == nullSid)
        {
            continue;
        }

        Sid_t const   meshSid = g_scene.getNodeBySid(osid).getSid();
        Mesh_s const &mesh    = g_handleTable.getMesh(meshSid);
        AABB          obsBox  = globalSpaceBB(g_scene.getNodeBySid(osid), mesh.box);
        obsBox.mm.min.z       = -10.f;
        obsBox.mm.max.z       = 10.f;

        Hit_t const hit = intersect(nRay, obsBox);
        if (hit.isect && hit.t < closestT)
        {
            closestT             = hit.t;
            foundInDestructables = false; // Closest hit is now from obstacles
            printf("[ScrollingTerrain] There's a nearer obstacle, no destruct\n");
            break;
        }
    }

    // If the closest intersection was from destructables, set it to nullSid
    if (foundInDestructables)
    {
        printf("[ScrollingTerrain] destructing one piece\n");
        auto it = std::find(m_destructables.begin(), m_destructables.end(), closestDestructableSid);
        if (it != m_destructables.end())
        {
            g_scene.removeNode(*it);
            *it = nullSid; // Set to nullSid
        }
        return true;
    }

    return false;
#else
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
#endif
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
{ //
    g_scene.removeNode(it->second);
    m_coinMap.erase(it);
}

void ScrollingTerrain::removeCoin(CoinMap::const_iterator const &it)
{ //
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
    { //
        g_eventQueue.emit(evMagnetAcquired, evData);
    }
    else if (sid == m_speedPowerUp)
    { //
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

bool Player::intersectPlayerWith(ScrollingTerrain &terrain)
{
    AABB const      playerBox{ globalSpaceBB(g_scene.getNodeBySid(m_sid), m_mesh->box) };
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
                { //
                    terrain.powerUpAcquired(index);
                    printf("[Player] POWER UP UP UP\n");
                    return false;
                }
            }
            ++index;
        }
    }

    return m_intersected;
}

void Player::incrementScore(U32_t increment, U32_t numCoins)
{ //
    m_score += static_cast<U64_t>(increment) * numCoins;
}

U64_t Player::getCurrentScore() const
{ // getter
    return m_score;
}

F32_t Player::getVelocity() const
{ //
    return (m_invincible ? speedBoost : 1.f) * glm::min(baseVelocity + m_velocityIncrement, maxBaseVelocity);
}

Player::U &Player::U::operator=(const Player::U &other)
{
    std::copy(std::begin(other.arr), std::end(other.arr), std::begin(arr));
    return *this;
}
} // namespace cge