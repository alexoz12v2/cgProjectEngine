#include "Player.h"

#include "Core/Event.h"
#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Core/Random.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Entity/CollisionWorld.h"
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

template<std::integral T> consteval U32_t numDigits(T value)
{
    U32_t digits = 1;
    while (value > 9)
    {
        value /= 10;
        digits++;
    }
    return digits;
}

template<U32_t N> struct FixedString
{
    constexpr Char8_t const *cStr() const { return buf; }
    Char8_t                  buf[N];
};

FixedString<numDigits(std::numeric_limits<U64_t>::max()) + 1>
  strFromIntegral(U64_t num)
{
    static U32_t constexpr maxLen =
      numDigits(std::numeric_limits<U64_t>::max());
    FixedString<maxLen + 1> res;
    sprintf(res.buf, "%zu\0", num);

    return res;
}

void Player::spawn(const Camera_t &view, Sid_t meshSid)
{
    EventArg_t listenerData{};
    listenerData.idata.p = reinterpret_cast<Byte_t *>(this);
    g_eventQueue.addListener(evKeyPressed, KeyCallback<Player>, listenerData);
    g_eventQueue.addListener(
      evMouseButtonPressed, mouseButtonCallback<Player>, listenerData);
    g_eventQueue.addListener(
      evFramebufferSize, framebufferSizeCallback<Player>, listenerData);

    m_sid  = meshSid;
    m_mesh = g_handleTable.get(meshSid);
    m_box  = m_mesh.asMesh().box;
    m_node = g_scene.getNodeBySid(m_sid);

    m_camera = view;

    CollisionObj_t cubeCollisionMesh = { .ebox =
                                           recomputeGlobalSpaceBB(m_sid, m_box),
                                         .sid = m_sid };

    m_worldObjPtr = g_world.addObject(cubeCollisionMesh);

    m_node->transform(
      glm::inverse(m_camera.viewTransform())
      * glm::translate(glm::mat4(1.F), meshCameraOffset));
}

#if 0
glm::vec3 Player::displacementTick(F32_t deltaTime)
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

void Player::onTick(F32_t deltaTime)
{
    static F32_t constexpr eps = std::numeric_limits<F32_t>::epsilon();
    // printf("Player::onTick deltaTime = %f\n", deltaTime);
    //  update bounding box
    glm::vec3 const displacement = displacementTick(deltaTime);
    if (!smallOrZero(displacement) && !m_intersected)
    {
        m_score += glm::max(
          static_cast<decltype(1ULL)>(displacement.y * scoreMultiplier),
          1ULL);
        m_worldObjPtr->ebox = recomputeGlobalSpaceBB(m_sid, m_box);

        // TODO remove when chunking
        m_lastDisplacement = displacement;

        // TODO add effective movement
        m_camera.position += displacement;

        m_node->transform(glm::translate(glm::mat4(1.f), displacement));

        auto const meshCenter = centroid(m_worldObjPtr->ebox);

        Ray_t const ray{ .o = m_camera.position, .d = glm::vec3(0, 1, 0) };
        Hit_t       hit;
        g_world.build();
        m_velocityMultiplier += deltaTime * multiplierTimeConstant;
    }
    else if (m_intersected)
    { //
        printf("[Player] INTERSECTION\n");
    }

    if (glm::abs(m_camera.position.x - m_targetXPos) > eps)
    {
        auto old  = m_camera.position.x;
        auto func = m_targetXPos > old ? glm::min<float> : glm::max<float>;
        auto disp = func((m_targetXPos - old) * baseShiftVelocity * deltaTime, m_targetXPos - old);
        printf("[Player] deltaTime: %f\n", deltaTime);

        if (glm::abs(m_camera.position.x - m_targetXPos) <= 0.5f)
        {
            disp = m_targetXPos - old;
        }

        m_camera.position.x += disp;
        m_node->transform(
          glm::translate(glm::mat4(1.f), glm::vec3(disp, 0.f, 0.f)));
    }

    auto const      fixedStr = strFromIntegral(m_score);
    glm::vec3 const xyScale{ m_framebufferSize.x / 1.95f,
                             m_framebufferSize.y / 1.2f,
                             1.f };
    glm::vec3 const color{ 0.2f, 0.2f, 0.2f };
    g_renderer2D.renderText(fixedStr.cStr(), xyScale, color);
}
#else

glm::vec3 Player::displacementTick(F32_t deltaTime)
{
    F32_t const      velocity  = glm::min(baseVelocity + m_velocityIncrement, maxBaseVelocity);

    // Calculate the movement direction based on camera's forward vector
    glm::vec3 direction = m_camera.forward;
    if (direction != glm::vec3(0.F)) { direction = glm::normalize(direction); }
    glm::vec3 const displacement = velocity * direction * deltaTime;

    return displacement;
}

void Player::onTick(F32_t deltaTime)
{
    printf("[Player] deltaTime = %f\n", deltaTime);
    if (m_intersected)
    { //
        EventArg_t evData{};
        evData.idata.u64 = m_score;
        printf("[Player] INTERSECTED\n");
    }

    glm::vec3 const displacement = displacementTick(deltaTime);
    m_score += glm::max(static_cast<decltype(1ULL)>(displacement.y * scoreMultiplier), 1ULL);

    m_lastDisplacement = displacement;

    m_camera.position += displacement;

    if (glm::abs(m_camera.position.x - m_targetXPos) > std::numeric_limits<F32_t>::epsilon())
    {
        auto old  = m_camera.position.x;
        F32_t disp = glm::mix(old, m_targetXPos, 1.f - glm::pow(baseShiftVelocity, deltaTime));

        if (glm::abs(disp - m_targetXPos) <= 0.5f)
        {
            disp = m_targetXPos;
        }
        m_camera.position.x = disp;
    }

    //g_scene.getNodeBySid(m_sid).translate(displacement);
    g_scene.getNodeBySid(m_sid)->setTransform(glm::translate(glm::inverse(m_camera.viewTransform()), meshCameraOffset));
    m_velocityIncrement = glm::max(glm::abs(glm::log(static_cast<F32_t>(m_score))), 1.f);
}

#endif

void Player::onFramebufferSize(I32_t width, I32_t height)
{
    using V             = decltype(m_framebufferSize)::value_type;
    m_framebufferSize.x = static_cast<V>(width);
    m_framebufferSize.y = static_cast<V>(height);
}

void Player::onKey(I32_t key, I32_t action)
{
    if (action == action::CGE_PRESS)
    {
        switch (key)
        {
        case key::CGE_KEY_A:
            if ((m_lane & LANE_LEFT) == 0)
            {
                m_lane <<= 1;
                m_targetXPos -= laneShift;
            }
            break;
        case key::CGE_KEY_D:
            if ((m_lane & LANE_RIGHT) == 0)
            {
                m_lane >>= 1;
                m_targetXPos += laneShift;
            }
            break;
        default:
            break;
        }
    }
}

void Player::onMouseButton([[maybe_unused]] I32_t key, I32_t action)
{
    if (action == action::CGE_PRESS) {}
    else if (action == action::CGE_RELEASE) {}
}

AABB_t Player::boundingBox() const { return m_box; }

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
    return centroid(recomputeGlobalSpaceBB(m_node, m_mesh.asMesh().box));
}

void ScrollingTerrain::init(
  Scene_s                                &scene,
  std::pmr::vector<Sid_t>::const_iterator begin,
  std::pmr::vector<Sid_t>::const_iterator end)
{
    static std::array<SceneNode_s *, numLanes> constexpr arr = { nullptr };
    glm::mat4 const identity                                 = glm::mat4(1.f);
    printf("ScrollingTerrain::init\n");
    // fill the m_pieces array and
    // trasform all pieces such that the middle one is in the origin

    U32_t const offset = std::distance(begin, end) / 2;
    U32_t       index  = 0;
    for (auto it = begin; it != end; ++it)
    {
        F32_t const     yOff = (static_cast<F32_t>(index) - offset) * pieceSize;
        glm::mat4 const t = glm::translate(identity, glm::vec3(0.f, yOff, 0.f));

        m_pieces.push_back(scene.addChild(*it, t));
        m_obstacles.push_back(arr);
        printf("adding object at y %f\n", yOff);

        ++index;
    }
}

std::pmr::deque<std::array<SceneNode_s *, numLanes>>
  ScrollingTerrain::updateTilesFromPosition(
    glm::vec3                      position,
    std::pmr::vector<Sid_t> const &sidSet,
    std::pmr::vector<Sid_t> const &obstacles)
{
    static std::array<SceneNode_s *, numLanes> constexpr arr = { nullptr };
    glm::mat4 const identity                                 = glm::mat4(1.f);
    // Calculate number of pieces in the terrain
    U32_t numPieces = m_pieces.size();

    // Check for empty terrain
    if (numPieces == 0)
    { //
        return m_obstacles;
    }

    // Calculate the "middle" tile index (halfway point of the vector)
    U32_t middleTile = numPieces / 2;
    F32_t middleYOff = m_pieces[middleTile]->getAbsoluteTransform()[3][1];

    // Calculate the difference between player tile and middle tile
    I32_t difference =
      static_cast<I32_t>((position.y - middleYOff) / pieceSize);

    // Handle potential negative difference for modulo operation
    difference = (difference + numPieces) % numPieces;

    // Update terrain tiles if there's a difference
    if (difference != 0)
    {
        // Number of tiles to move (absolute difference)
        U32_t numToMove = std::abs(difference);
        U32_t numPieces = m_pieces.size();
        F32_t yOffset   = numPieces * pieceSize;

        // Temporary storage for moved pieces
        std::vector<SceneNode_s *> temp(numToMove);

        // Move tiles from back to front (if difference is positive)
        if (difference > 0)
        {
            // Copy elements from the back
            std::copy(
              m_pieces.begin(), m_pieces.begin() + numToMove, temp.begin());

            // Remove elements from the front
            m_pieces.erase(m_pieces.begin(), m_pieces.begin() + numToMove);

            // translate each piece in temp
            for (U32_t i = 0; i != temp.size(); ++i)
            {
                temp[i]->transform(glm::translate(
                  identity, glm::vec3(0.f, yOffset - i * pieceSize, 0.f)));
            }

            // Insert elements at the back
            m_pieces.insert(m_pieces.end(), temp.begin(), temp.end());
        }
        else
        {
            // Move tiles from front to back (if difference is negative)
            // Copy elements from the front
            std::copy(m_pieces.end() - numToMove, m_pieces.end(), temp.begin());

            // Remove elements from the back
            m_pieces.erase(m_pieces.end() - numToMove, m_pieces.end());

            // translate each piece in temp
            for (U32_t i = 0; i != temp.size(); ++i)
            {
                temp[i]->transform(glm::translate(
                  identity, glm::vec3(0.f, -yOffset + i * pieceSize, 0.f)));
            }

            // Insert elements at the front
            m_pieces.insert(m_pieces.begin(), temp.begin(), temp.end());
        }

        for (auto pNode : temp)
        {
            // Update SIDs of moved pieces
            U32_t const setIndex = g_random.next<U32_t>() % temp.size();
            Sid_t const sid      = sidSet[setIndex];
            pNode->setSid(sid);

            //m_obstacles.pop_front();
            //m_obstacles.push_back(arr);
            //auto &back = m_obstacles.back();
            //// TODO: spawn props in the new tiles
            //if (!obstacles.empty())
            //{
            //    U32_t              numObstacles = g_random.next<U32_t>() % 3;
            //    std::vector<F32_t> availableLanes{ -laneShift, 0, laneShift };
            //    for (U32_t i = 0; i != numObstacles; ++i)
            //    {
            //        U32_t const index =
            //          g_random.next<U32_t>() % availableLanes.size();
            //        F32_t const positionX = availableLanes[index];
            //        F32_t const positionYFromPieceCenter =
            //          (g_random.next<F32_t>() - 0.5f) * pieceSize / 2.3f;
            //        availableLanes.erase(availableLanes.begin() + index);

            //        Sid_t const obstacle =
            //          obstacles[g_random.next<U32_t>() % obstacles.size()];
            //        auto const transform = glm::translate(
            //          pNode->getAbsoluteTransform(),
            //          glm::vec3(positionX, positionYFromPieceCenter, 0.f));

            //        back[i] = g_scene.addChild(obstacle, transform);

            //        CollisionObj_t collision = {
            //            .ebox = recomputeGlobalSpaceBB(
            //              obstacle,
            //              computeAABB(g_handleTable.get(obstacle).asMesh())),
            //            .sid = obstacle
            //        };

            //        g_world.addObject(collision);
            //    }
            //}
        }
#if defined(CGE_DEBUG)
        for (auto pNode : m_pieces)
        {
            auto  mat  = pNode->getAbsoluteTransform();
            F32_t yOff = mat[3][1];
            printf("piece at %f\n", yOff);
        }
        printf("\n\n");
#endif
    }

    return m_obstacles;
}

bool Player::intersectPlayerWith(std::pmr::deque<std::array<SceneNode_s *, numLanes>> const & obstacles, Hit_t & outHit)
{
    bool res = false;
    if (!obstacles.empty())
    {
        auto const playerBox = recomputeGlobalSpaceBB(m_sid, m_box);
        for (auto const &obsArr : obstacles)
        {
            for (auto const *pNode : obsArr)
            {
                if (!pNode)
                { //
                    break;
                }
                auto const &mesh = g_handleTable.get(pNode->getSid()).asMesh();
                auto const  box  = mesh.box;
                auto const  obsBox = recomputeGlobalSpaceBB(pNode, box);
                res |= intersect(playerBox, obsBox);
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
}