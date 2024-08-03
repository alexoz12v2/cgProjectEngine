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
    m_node = g_scene.getNodeBySid(m_sid); // optional initialized

    m_camera = view;

    (*m_node)->transform(
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
        m_score += glm::max(
          static_cast<decltype(m_score)>(displacement.y * scoreMultiplier),
          1ULL);

        m_lastDisplacement = displacement;

        m_camera.position += displacement;

        (*m_node)->transform(glm::translate(glm::mat4(1.f), displacement));

        auto const meshCenter =
          centroid(recomputeGlobalSpaceBB(*m_node, m_mesh->box));

        Ray_t const ray{ .o = m_camera.position, .d = glm::vec3(0, 1, 0) };
        Hit_t       hit;
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

        (*m_node)->transform(
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

AABB_t Player::boundingBox() const { return m_mesh->box; }

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
    return centroid(recomputeGlobalSpaceBB(*m_node, m_mesh->box));
}

void ScrollingTerrain::init(Scene_s &scene, std::span<Sid_t> pieces)
{
    static ObstacleList::value_type const arr      = { tl::nullopt };
    glm::mat4 const                       identity = glm::mat4(1.f);
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

        m_pieces[index]    = scene.addChild(*it, t);
        m_obstacles[index] = arr;

        ++index;
    }
}

ScrollingTerrain::ObstacleList const &ScrollingTerrain::updateTilesFromPosition(
  glm::vec3               position,
  std::span<Sid_t> const &sidSet,
  std::span<Sid_t> const &obstacles)
{
    // check if player moved one tile forward
    U32_t const     nextPieceIdx = (m_first + 1) % numPieces;
    glm::mat4 const pieceTransform =
      (*m_pieces[nextPieceIdx])->getAbsoluteTransform();
    glm::vec4 const piecePosition = pieceTransform[3];

    // if yes, then update first and last and traslate everything from first
    // to last
    if (position.y - piecePosition.y > static_cast<F32_t>(pieceSize >> 1))
    {
        (*m_pieces[m_first])
          ->transform(glm::translate(
            glm::mat4(1.f), glm::vec3(0.f, pieceSize * numPieces, 0.f)));

        // remove the obstacles from the moved piece
        for (U32_t i = 0; i < m_obstacles[m_first].size(); ++i)
        {
            if (m_obstacles[m_first][i].has_value())
            { // remove from scene
                g_scene.removeNode(*m_obstacles[m_first][i]);
                m_obstacles[m_first][i] = tl::nullopt;
            }
        }

        // add new obstacles in the moved piece
        U32_t const numObstacles = g_random.next<U32_t>(0, 2);

        std::array<F32_t, 3> arr = { -laneShift, 0, laneShift };
        std::ranges::shuffle(arr, g_random.getGen());

        for (U32_t idx = 0; idx < numObstacles; ++idx)
        {
            U32_t const obstacleIdx =
              g_random.next<U32_t>(0, obstacles.size() - 1);
            Sid_t const obstacleId    = obstacles[obstacleIdx];
            m_obstacles[m_first][idx] = g_scene.addChild(
              obstacleId,
              glm::translate(
                pieceTransform,
                glm::vec3(arr[idx], pieceSize * (numPieces - 1), 0.f)));
        }

        // maintain indices
        m_first = nextPieceIdx;
        m_last  = (m_last + 1) % numPieces;
    }

    return m_obstacles;
}

bool Player::intersectPlayerWith(
  std::span<std::array<tl::optional<SceneNodeHandle>, numLanes>> const
        &obstacles,
  Hit_t &outHit)
{
    bool res = false;
    if (!obstacles.empty())
    {
        auto const playerBox = recomputeGlobalSpaceBB(*m_node, m_mesh->box);
        for (auto const &obsArr : obstacles)
        {
            for (auto pNode : obsArr)
            {
                if (pNode.has_value() && pNode->isValid())
                {
                    auto const ref = g_handleTable.get((*pNode)->getSid());
                    if (ref.hasValue())
                    {
                        auto const &mesh  = ref.asMesh();
                        auto const  box   = mesh.box;
                        auto const obsBox = recomputeGlobalSpaceBB(*pNode, box);
                        res |= intersect(playerBox, obsBox);
                    }
                }
            }

            if (res)
            { //
                break;
            }
        }
    }

    m_intersected = res;
    return res;
}

void Player::setSwishSound(irrklang::ISoundSource *sound)
{ //
    m_swishSoundSource = sound;
}

} // namespace cge