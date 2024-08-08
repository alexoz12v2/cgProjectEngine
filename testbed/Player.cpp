#include "Player.h"

#include "ConstantsAndStructs.h"
#include "Core/Event.h"
#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Launch/Entry.h"
#include "Render/Renderer.h"
#include "Render/Renderer2d.h"
#include "Resource/HandleTable.h"
#include "Resource/Rendering/cgeMesh.h"
#include "Utils.h"

#include <glm/common.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

#include <iterator>

namespace cge
{

static AABB enlarge(AABB const &box)
{ //
    AABB res{ box };
    res.max.z = glm::max(box.max.z, 20.f);
    res.max.y += 10.f;
    return res;
}

Player::~Player()
{
    if (m_init)
    {
        for (auto const &pair : m_listeners.arr)
        { //
            g_eventQueue.removeListener(pair);
        }

        if (m_swishSound)
        { //
            m_swishSound->stop();
            m_swishSound->drop();
        }

        g_soundEngine()->removeSoundSource(m_swishSoundSource);
        g_scene.removeNode(m_sid);
    }
}

void Player::spawn(const Camera_t &view, Sid_t meshSid)
{
    EventArg_t listenerData{};
    listenerData.idata.p    = reinterpret_cast<Byte_t *>(this);
    m_listeners.keyListener = g_eventQueue.addListener(evKeyPressed, KeyCallback<Player>, listenerData);
    m_listeners.mouseButtonListener =
      g_eventQueue.addListener(evMouseButtonPressed, mouseButtonCallback<Player>, listenerData);
    m_listeners.framebufferSizeListener =
      g_eventQueue.addListener(evFramebufferSize, framebufferSizeCallback<Player>, listenerData);

    m_sid  = g_scene.addNode(meshSid);
    m_mesh = &g_handleTable.get(meshSid).asMesh();

    m_camera = view;

    g_scene.getNodeBySid(m_sid).transform(
      glm::inverse(m_camera.viewTransform()) * glm::translate(glm::mat4(1.F), glm::vec3(0, -2, -10)));

    m_swishSoundSource = g_soundEngine()->addSoundSourceFromFile("../assets/swish.mp3");

    m_init = true;
}

void Player::onTick(F32_t deltaTime)
{
    static F32_t constexpr eps   = std::numeric_limits<F32_t>::epsilon();
    glm::vec3 const displacement = displacementTick(deltaTime);
    if (!smallOrZero(displacement) && !m_intersected)
    {
        auto const meshCenter = centroid(globalSpaceBB(g_scene.getNodeBySid(m_sid), m_mesh->box));
        m_oldPosition         = meshCenter;
        m_score += glm::max(static_cast<decltype(m_score)>(displacement.y * scoreMultiplier), 1ULL);

        m_lastDisplacement = displacement;

        m_camera.position += displacement;

        g_scene.getNodeBySid(m_sid).transform(glm::translate(glm::mat4(1.f), displacement));

        m_velocityMultiplier += deltaTime * multiplierTimeConstant;
    }
    else if (m_intersected)
    { //
        EventArg_t evData{};
        evData.idata.u64 = m_score;
        g_eventQueue.emit(evGameOver, evData);
    }

    if (glm::abs(m_camera.position.x - m_targetXPos) > eps)
    {
        auto old  = m_camera.position.x;
        auto disp = (m_targetXPos - old) * baseShiftVelocity * 0.004f; //* deltaTime;

        m_camera.position.x += disp;

        if (glm::abs(m_camera.position.x - m_targetXPos) <= 0.5f)
        {
            m_camera.position.x = m_targetXPos;
            disp                = m_camera.position.x - old;
            if (m_swishSound)
            {
                m_swishSound->stop();
                m_swishSound->drop();
                m_swishSound = nullptr;
            }
        }

        g_scene.getNodeBySid(m_sid).transform(glm::translate(glm::mat4(1.f), glm::vec3(disp, 0.f, 0.f)));
    }

    auto const      fixedStr = strFromIntegral(m_score);
    glm::vec3 const xyScale{ m_framebufferSize.x / 1.95f, m_framebufferSize.y / 1.2f, 1.f };
    glm::vec3 const color{ 0.2f, 0.2f, 0.2f };
    g_renderer2D.renderText(fixedStr.cStr(), xyScale, color);
}

void Player::onFramebufferSize(I32_t width, I32_t height)
{
    using V             = decltype(m_framebufferSize)::value_type;
    m_framebufferSize.x = static_cast<V>(width);
    m_framebufferSize.y = static_cast<V>(height);
}

glm::vec3 Player::displacementTick(F32_t deltaTime) const
{
    F32_t const velocity = glm::min(baseVelocity * m_velocityMultiplier, maxBaseVelocity) * deltaTime;

    // Calculate the movement direction based on camera's forward vector
    glm::vec3 direction = m_camera.forward;
    if (direction != glm::vec3(0.F)) { direction = glm::normalize(direction); }
    glm::vec3 const displacement = velocity * direction;

    return displacement;
}

void Player::onKey(I32_t key, I32_t action)
{
    static F32_t constexpr eps = std::numeric_limits<F32_t>::epsilon();
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
        case key::SPACE:
        {
            EventArg_t evArg{};
            evArg.fdata.f32[0] = m_oldPosition[0];
            evArg.fdata.f32[1] = m_oldPosition[1];
            evArg.fdata.f32[2] = m_oldPosition[2];
            evArg.idata.i16[0] = 0;
            evArg.idata.i16[1] = 1;
            evArg.idata.i16[2] = 0;
            g_eventQueue.emit(evShoot, evArg);
            break;
        }
        default:
            break;
        }
    }
}

void Player::onMouseButton([[maybe_unused]] I32_t key, I32_t action)
{
    if (action == action::PRESS) {}
    else if (action == action::RELEASE) {}
}

AABB Player::boundingBox() const { return m_mesh->box; }

void Player::yawPitchRotate(F32_t yaw, F32_t pitch)
{
    static F32_t constexpr maxPitch = 89.F;

    yaw   = glm::radians(yaw);
    pitch = glm::radians(glm::clamp(pitch, -maxPitch, maxPitch));

    glm::vec3 direction;
    direction.x = glm::cos(yaw) * glm::cos(pitch);
    direction.y = glm::sin(yaw) * glm::cos(pitch);
    direction.z = -glm::sin(pitch);

    m_camera.forward = glm::normalize(direction);

    // Assuming the initial up direction is the z-axis
    auto worldUp = glm::vec3(0.0F, 0.0F, 1.0F);

    // Calculate the right and up vectors using cross products
    m_camera.right = glm::normalize(glm::cross(worldUp, m_camera.forward));
    m_camera.up    = glm::normalize(glm::cross(m_camera.forward, m_camera.right));
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
    glm::mat4 const identity      = glm::mat4(1.f);
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
}

void ScrollingTerrain::updateTilesFromPosition(glm::vec3 position)
{
    // check if player moved one tile forward
    U32_t const     nextPieceIdx   = (m_first + 1) % numPieces;
    glm::mat4 const pieceTransform = g_scene.getNodeBySid(m_pieces[nextPieceIdx]).getTransform();
    glm::vec4 const piecePosition  = pieceTransform[3];

    // if yes, then update first and last and traslate everything from first to last
    if (position.y - piecePosition.y > static_cast<F32_t>(pieceSize >> 1))
    {
        glm::vec3 displacement{ 0.f, pieceSize * numPieces, 0.f };
        g_scene.getNodeBySid(m_pieces[m_first]).transform(glm::translate(glm::mat4(1.f), displacement));

        // remove the obstacle from the moved piece
        if (m_obstacles[m_first] != nullSid)
        { // remove from scene
            g_scene.removeNode(m_obstacles[m_first]);
            m_obstacles[m_first] = nullSid;
        }

        // add new obstacles in the moved piece
        U32_t const hasObstacle = g_random.next<U32_t>(0, 1);
        if (hasObstacle)
        { //
            addPropOfType(g_random.next<U32_t>(0, 1), pieceTransform);
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

void ScrollingTerrain::handleShoot(Ray const &ray)
{
    ObstacleList                   list{ nullSid };
    std::array<F32_t, list.size()> yPositions{ 0.f };
    std::array<U32_t, list.size()> indices{ static_cast<U32_t>(-1) };
    U32_t                          index         = 0;
    U32_t                          originalIndex = 0;

    for (Sid_t &dsid : m_destructables)
    {
        if (dsid != nullSid)
        {
            Sid_t                meshSid{ g_scene.getNodeBySid(dsid).getSid() };
            HandleTable_s::Ref_s ref{ g_handleTable.get(meshSid) };
            if (ref.hasValue())
            {
                auto const &mesh   = ref.asMesh();
                auto const  box    = mesh.box;
                auto const  obsBox = enlarge(globalSpaceBB(g_scene.getNodeBySid(dsid), box));
                Hit_t const res    = intersect(ray, obsBox);
                if (res.isect)
                {
                    list[index]         = dsid;
                    indices[index]      = originalIndex;
                    yPositions[index++] = res.p.y;
                }
            }
        }

        ++originalIndex;
    }

    if (index != 0)
    {
        auto  it     = std::min_element(yPositions.cbegin(), yPositions.cbegin() + index);
        U32_t minIdx = std::distance(yPositions.cbegin(), it);
        assert(minIdx >= 0 && minIdx < list.size());
        g_scene.removeNode(list[minIdx]);
        m_destructables[indices[minIdx]] = nullSid;
    }
}

ScrollingTerrain::ObstacleList const &ScrollingTerrain::getObstacles() const
{ // getter
    return m_obstacles;
}

ScrollingTerrain::ObstacleList const &ScrollingTerrain::getDestructables() const
{ // getter
    return m_destructables;
}

std::pmr::unordered_map<U32_t, Sid_t> const &ScrollingTerrain::getCoinMap() const
{ // getter
    return m_coinMap;
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
    std::array<F32_t, 3> arr = { -laneShift, 0, laneShift };
    std::ranges::shuffle(arr, g_random.getGen());
    if (type == 0)
    { // choose an obstacle and spawn it
        U32_t const obstacleIdx = g_random.next<U32_t>(0, m_obstacleSetSize - 1);
        Sid_t const sid         = m_obstacleSet[obstacleIdx];
        m_obstacles[m_first]    = g_scene.addNode(sid);
        g_scene.getNodeBySid(m_obstacles[m_first])
          .transform(glm::translate(pieceTransform, glm::vec3(arr[0], pieceSize * (numPieces - 1), 0.f)));
    }
    else if (type == 1)
    { // choose a destructable and spawn it
        U32_t const destructableIdx = g_random.next<U32_t>(0, m_destructableSetSize - 1);
        Sid_t const sid             = m_destructableSet[destructableIdx];
        m_destructables[m_first]    = g_scene.addNode(sid);
        g_scene.getNodeBySid(m_destructables[m_first])
          .transform(glm::translate(pieceTransform, glm::vec3(arr[0], pieceSize * (numPieces - 1), 0.f)));
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
    F32_t const   coinDepth = coinMesh.box.max.y - coinMesh.box.min.y;
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
    for (auto it = m_coinMap.begin(); it != m_coinMap.end();)
    { // if the coin is in the y range of the piece begin removed, then remove it
        if (it->first <= threshold)
        { //
            Sid_t sceneSid = it->second;
            g_scene.removeNode(sceneSid);
            it = m_coinMap.erase(it);
        }
        else
        { //
            ++it;
        }
    }
}


bool Player::intersectPlayerWith(ScrollingTerrain const &terrain)
{
    AABB const      playerBox{ globalSpaceBB(g_scene.getNodeBySid(m_sid), m_mesh->box) };
    glm::vec3 const newPosition{ centroid(playerBox) };
    Ray const       playerRay{ m_oldPosition, newPosition - m_oldPosition };
    F32_t const     movementDistance = newPosition.y - m_oldPosition.y;

    m_intersected = false;
    for (auto const &pObs : terrain.getObstacles())
    {
        HandleTable_s::Ref_s ref{ nullRef };
        Sid_t                meshSid{ nullSid };
        if (pObs != nullSid && (ref = g_handleTable.get(meshSid = g_scene.getNodeBySid(pObs).getSid())).hasValue())
        {
            auto const &mesh   = ref.asMesh();
            auto const  box    = mesh.box;
            auto const  obsBox = enlarge(globalSpaceBB(g_scene.getNodeBySid(pObs), box));
            Hit_t const res    = intersect(playerRay, obsBox);
            if (res.isect && (res.t <= movementDistance && res.p.y > m_oldPosition.y))
            {
                m_intersected = true;
                return m_intersected;
            }
        }
    }

    for (auto const &pObs : terrain.getDestructables())
    {
        HandleTable_s::Ref_s ref{ nullRef };
        Sid_t                meshSid{ nullSid };
        if (pObs != nullSid && (ref = g_handleTable.get(meshSid = g_scene.getNodeBySid(pObs).getSid())).hasValue())
        {
            auto const &mesh   = ref.asMesh();
            auto const  box    = mesh.box;
            auto const  obsBox = enlarge(globalSpaceBB(g_scene.getNodeBySid(pObs), box));
            Hit_t const res    = intersect(playerRay, obsBox);
            if (res.isect && (res.t <= movementDistance && res.p.y > m_oldPosition.y))
            {
                m_intersected = true;
                return m_intersected;
            }
        }
    }

    return m_intersected;
}

void Player::setSwishSound(irrklang::ISoundSource *sound)
{ //
    m_swishSoundSource = sound;
}

void Player::incrementScore(U32_t increment)
{ //
    m_score += increment;
}

} // namespace cge