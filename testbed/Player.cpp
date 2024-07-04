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
void Player::spawn(const Camera_t &view, Sid_t meshSid)
{
    EventArg_t listenerData{};
    listenerData.idata.p = reinterpret_cast<Byte_t *>(this);
    g_eventQueue.addListener(evKeyPressed, KeyCallback<Player>, listenerData);
    g_eventQueue.addListener(
      evMouseButtonPressed, mouseButtonCallback<Player>, listenerData);
    g_eventQueue.addListener(
      evMouseMoved, mouseMovementCallback<Player>, listenerData);

    m_sid  = meshSid;
    m_mesh = g_handleTable.get(meshSid);
    m_box  = computeAABB(m_mesh.asMesh());

    m_camera = view;

    CollisionObj_t cubeCollisionMesh = { .ebox = recomputeGlobalSpaceBB(),
                                         .sid  = m_sid };

    m_worldObjPtr = g_world.addObject(cubeCollisionMesh);
    g_scene.getNodeBySid(m_sid)->transform(
      glm::inverse(m_camera.viewTransform())
      * glm::translate(glm::mat4(1.F), glm::vec3(0, -2, -10)));
}

AABB_t Player::recomputeGlobalSpaceBB() const
{
    glm::mat4 transform = g_scene.getNodeBySid(m_sid)->getAbsoluteTransform();

    AABB_t const gSpaceAABB = { .min = transform * glm::vec4(m_box.min, 1.f),
                                .max = transform * glm::vec4(m_box.max, 1.f) };

    return gSpaceAABB;
}

void Player::onTick(F32_t deltaTime)
{
    // printf("Player::onTick deltaTime = %f\n", deltaTime);
    //  update bounding box
    glm::vec3 const displacement = displacementTick(deltaTime);
    if (!smallOrZero(displacement))
    {
        printf(
          "displacement: %f %f %f\n",
          displacement.x,
          displacement.y,
          displacement.z);

        m_worldObjPtr->ebox = recomputeGlobalSpaceBB();

        // TODO remove when chunking
        m_lastDisplacement = displacement;

        // TODO add effective movement
        m_camera.position += displacement;

        g_scene.getNodeBySid(m_sid)->transform(
          glm::translate(glm::mat4(1.f), displacement));
        // g_scene.getNodeBySid(m_sid)->transform(
        //   glm::inverse(m_camera.viewTransform()));
    }

    auto const meshCenter = centroid(m_worldObjPtr->ebox);

    Ray_t const ray{ .o = m_camera.position,
                     .d = meshCenter - m_camera.position };
    Hit_t       hit;
    g_world.build();
    if (g_world.intersect(ray, 0, hit))
    {
        // if (hit.t <= 0.5F) { m_camera.position = oldPosition; }
    }
}

glm::vec3 Player::displacementTick(F32_t deltaTime) const
{
    F32_t const velocity = baseVelocity * deltaTime;

    // Calculate the movement direction based on camera's forward vector
    glm::vec3 direction = (F32_t)m_keyPressed[0] * m_camera.forward
                          + (F32_t)m_keyPressed[1] * m_camera.right;
    if (direction != glm::vec3(0.F)) { direction = glm::normalize(direction); }
    glm::vec3 const displacement = velocity * direction;

    return displacement;
}

void Player::onKey(I32_t key, I32_t action)
{
    I32_t directionSign = 0;
    if (action == GLFW_PRESS) { directionSign = 1; }
    if (action == GLFW_RELEASE) { directionSign = -1; }

    switch (key)
    {
    case GLFW_KEY_W:
        m_keyPressed[0] += directionSign;
        break;
    case GLFW_KEY_A:
        m_keyPressed[1] += directionSign;
        break;
    case GLFW_KEY_S:
        m_keyPressed[0] -= directionSign;
        break;
    case GLFW_KEY_D:
        m_keyPressed[1] -= directionSign;
        break;
    default:
        break;
    }
}

void Player::onMouseButton([[maybe_unused]] I32_t key, I32_t action)
{
    if (action == GLFW_PRESS)
    {
        disableCursor();
        m_isCursorDisabled = true;
    }
    else if (action == GLFW_RELEASE)
    {
        enableCursor();
        m_isCursorDisabled = false;
    }
}

void Player::onMouseMovement(F32_t xpos, F32_t ypos)
{
    // TODO refactor
    auto transform     = g_scene.getNodeBySid(m_sid)->getAbsoluteTransform();
    static F32_t yaw   = 0;
    static F32_t pitch = 0;
    if (!m_isCursorDisabled)
    {
        m_lastCursorPosition = { xpos, ypos };
        return;
    }

    F32_t const deltaX = m_lastCursorPosition.x - xpos;
    F32_t const deltaY = ypos - m_lastCursorPosition.y;
    yaw += deltaX * mouseSensitivity;
    pitch += deltaY * mouseSensitivity;

    yawPitchRotate(yaw, pitch);
    auto const center =
      m_camera.position + glm::vec3(m_camera.forward.x, 0, m_camera.forward.z);
    glm::lookAt(m_camera.position, center, glm::vec3(0, 0, 1));

    m_lastCursorPosition = { xpos, ypos };
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
glm::vec3 Player::getCentroid() const { return centroid(m_worldObjPtr->ebox); }

void ScrollingTerrain::init(
  Scene_s                                &scene,
  std::pmr::vector<Sid_t>::const_iterator begin,
  std::pmr::vector<Sid_t>::const_iterator end)
{
    glm::mat4 const identity = glm::mat4(1.f);
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
        printf("adding object at y %f\n", yOff);

        ++index;
    }
}

void ScrollingTerrain::updateTilesFromPosition(
  glm::vec3                      position,
  std::pmr::vector<Sid_t> const &sidSet)
{
    glm::mat4 const identity = glm::mat4(1.f);
    // Calculate number of pieces in the terrain
    U32_t numPieces = m_pieces.size();

    // Check for empty terrain
    if (numPieces == 0)
    {
        return; // Nothing to update
    }

    // Calculate the "middle" tile index (halfway point of the vector)
    U32_t middleTile = numPieces / 2;
    F32_t middleYOff = m_pieces[middleTile]->getAbsoluteTransform()[3][1];

    // Calculate player tile index (using integer division)
    U32_t playerTile =
      static_cast<U32_t>((position.y - middleYOff) / pieceSize) + middleTile;

    // Calculate the difference between player tile and middle tile
    int difference = static_cast<int>(playerTile - middleTile);

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

        // Update SIDs of moved pieces
        for (auto pNode : temp)
        {
            U32_t setIndex = g_random.next<U32_t>() % temp.size();
            Sid_t sid      = sidSet[setIndex];
            pNode->setSid(sid);
        }

        // TODO: spawn props in the new tiles

        for (auto pNode : m_pieces)
        {
            auto  mat  = pNode->getAbsoluteTransform();
            F32_t yOff = mat[3][1];
            printf("piece at %f\n", yOff);
        }
        printf("\n\n");
    }
}

} // namespace cge