#include "Player.h"

#include "ConstantsAndStructs.h"
#include "Core/Event.h"
#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Core/Random.h"
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
    }
}

void Player::spawn(const Camera_t &view, Sid_t meshSid)
{
    EventArg_t listenerData{};
    listenerData.idata.p = reinterpret_cast<Byte_t *>(this);
    m_listeners.keyListener =
      g_eventQueue.addListener(evKeyPressed, KeyCallback<Player>, listenerData);
    m_listeners.mouseButtonListener = g_eventQueue.addListener(
      evMouseButtonPressed, mouseButtonCallback<Player>, listenerData);
    m_listeners.framebufferSizeListener = g_eventQueue.addListener(
      evFramebufferSize, framebufferSizeCallback<Player>, listenerData);

    m_sid  = meshSid;
    m_mesh = &g_handleTable.get(meshSid).asMesh();
    m_node = g_scene.getNodePairByNodeRef(g_scene.getNodeBySid(m_sid));

    m_camera = view;

    m_node->second.transform(
      glm::inverse(m_camera.viewTransform())
      * glm::translate(glm::mat4(1.F), glm::vec3(0, -2, -10)));

    m_swishSoundSource =
      g_soundEngine()->addSoundSourceFromFile("../assets/swish.mp3");

    m_init = true;
}

void Player::onTick(F32_t deltaTime)
{
    static F32_t constexpr eps   = std::numeric_limits<F32_t>::epsilon();
    glm::vec3 const displacement = displacementTick(deltaTime);
    if (!smallOrZero(displacement) && !m_intersected)
    {
        auto const meshCenter =
          centroid(globalSpaceBB(m_node->second, m_mesh->box));
        m_oldPosition = meshCenter;
        m_score += glm::max(
          static_cast<decltype(m_score)>(displacement.y * scoreMultiplier),
          1ULL);

        m_lastDisplacement = displacement;

        m_camera.position += displacement;

        m_node->second.transform(glm::translate(glm::mat4(1.f), displacement));

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
        auto old = m_camera.position.x;
        auto disp =
          (m_targetXPos - old) * baseShiftVelocity * 0.004f; //* deltaTime;
        printf("[Player] deltaTime: %f\n", deltaTime);

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

        m_node->second.transform(
          glm::translate(glm::mat4(1.f), glm::vec3(disp, 0.f, 0.f)));
    }

    auto const      fixedStr = strFromIntegral(m_score);
    glm::vec3 const xyScale{ m_framebufferSize.x / 1.95f,
                             m_framebufferSize.y / 1.2f,
                             1.f };
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
    F32_t const velocity =
      glm::min(baseVelocity * m_velocityMultiplier, maxBaseVelocity)
      * deltaTime;

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
    m_camera.up = glm::normalize(glm::cross(m_camera.forward, m_camera.right));
}
glm::mat4 Player::viewTransform() const { return m_camera.viewTransform(); }
Camera_t  Player::getCamera() const { return m_camera; }
glm::vec3 Player::lastDisplacement() const { return m_lastDisplacement; }
glm::vec3 Player::getCentroid() const
{
    return centroid(globalSpaceBB(m_node->second, m_mesh->box));
}

void ScrollingTerrain::init(Scene_s &scene, std::span<Sid_t> pieces)
{
    glm::mat4 const identity = glm::mat4(1.f);
    printf("ScrollingTerrain::init\n");
    // fill the m_pieces array and
    // trasform all pieces such that the middle one is in the origin

    static U32_t constexpr offset = 1;
    U32_t index                   = 0;
    for (auto it = pieces.begin(); it != pieces.end() && index < numPieces;
         ++it)
    {
        F32_t const     yOff = (static_cast<F32_t>(index) - offset) * pieceSize;
        glm::mat4 const t = glm::translate(identity, glm::vec3(0.f, yOff, 0.f));

        m_pieces[index] = scene.addNode(*it);
        m_pieces[index]->second.transform(t);
        m_obstacles[index] = tl::nullopt;

        ++index;
    }
}

void ScrollingTerrain::updateTilesFromPosition(
  glm::vec3               position,
  std::span<Sid_t> const &sidSet,
  std::span<Sid_t> const &obstacles)
{
    // check if player moved one tile forward
    U32_t const     nextPieceIdx = (m_first + 1) % numPieces;
    glm::mat4 const pieceTransform =
      m_pieces[nextPieceIdx]->second.getTransform();
    glm::vec4 const piecePosition = pieceTransform[3];

    // if yes, then update first and last and traslate everything from first
    // to last
    if (position.y - piecePosition.y > static_cast<F32_t>(pieceSize >> 1))
    {
        m_pieces[m_first]->second.transform(glm::translate(
          glm::mat4(1.f), glm::vec3(0.f, pieceSize * numPieces, 0.f)));

        // remove the obstacle from the moved piece
        if (m_obstacles[m_first].has_value())
        { // remove from scene
            g_scene.removeNode(m_obstacles[m_first]->first);
            m_obstacles[m_first] = tl::nullopt;
        }

        std::array<F32_t, 3> arr = { -laneShift, 0, laneShift };
        std::ranges::shuffle(arr, g_random.getGen());

        // add new obstacles in the moved piece
        U32_t const hasObstacle = g_random.next<U32_t>(0, 1);
        if (hasObstacle)
        {
            U32_t const obstacleIdx =
              g_random.next<U32_t>(0, obstacles.size() - 1);
            Sid_t const obstacleId = obstacles[obstacleIdx];
            m_obstacles[m_first]   = g_scene.addNode(obstacleId);
            m_obstacles[m_first]->second.transform(glm::translate(
              pieceTransform,
              glm::vec3(arr[0], pieceSize * (numPieces - 1), 0.f)));
        }

        // maintain indices
        m_first = nextPieceIdx;
        m_last  = (m_last + 1) % numPieces;
    }
}

ScrollingTerrain::ObstacleList const &ScrollingTerrain::getObstacles() const
{ // getter
    return m_obstacles;
}

static constexpr std::pair<HandleTable_s::Ref_s, B8_t>
  checkOptObstacle(tl::optional<Scene_s::PairNode &> const &opt)
{
    std::pair<HandleTable_s::Ref_s, B8_t> p{ std::make_pair(nullRef, false) };
    if (opt.has_value())
    {
        auto const ref = g_handleTable.get(opt->first);
        if (ref.hasValue())
        {
            p.first  = ref;
            p.second = true;
        }
    }

    return p;
}

bool Player::intersectPlayerWith(
  ScrollingTerrain::ObstacleList const &obstacles)
{
    AABB const      playerBox{ globalSpaceBB(m_node->second, m_mesh->box) };
    glm::vec3 const newPosition{ centroid(playerBox) };
    Ray const       playerRay{ m_oldPosition, newPosition - m_oldPosition };
    F32_t const     movementDistance = newPosition.y - m_oldPosition.y;

    m_intersected = false;
    for (auto const &pObs : obstacles)
    {
        if (auto const &p{ checkOptObstacle(pObs) }; p.second)
        {
            auto const &mesh   = p.first.asMesh();
            auto const  box    = mesh.box;
            auto const  obsBox = globalSpaceBB(pObs->second, box);
            Hit_t const res    = intersect(playerRay, obsBox);
            if (
              res.isect
              && (res.t <= movementDistance && res.p.y > m_oldPosition.y))
            {
                m_intersected = true;
                break;
            }
        }
    }

    return m_intersected;
}

void Player::setSwishSound(irrklang::ISoundSource *sound)
{ //
    m_swishSoundSource = sound;
}

} // namespace cge