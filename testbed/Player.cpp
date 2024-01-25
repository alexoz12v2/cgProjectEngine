#include "Player.h"

#include "Core/Event.h"
#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Entity/CollisionWorld.h"
#include "Launch/Entry.h"
#include "Render/Renderer.h"
#include "Resource/HandleTable.h"
#include "Resource/Rendering/cgeMesh.h"

#include <glm/common.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

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
}

AABB_t Player::recomputeGlobalSpaceBB() const
{
    glm::mat4 &transform = g_scene.getNodeBySid(m_sid)->absoluteTransform;

    transform = glm::inverse(m_camera.viewTransform())
                * glm::translate(glm::mat4(1.F), glm::vec3(0, 0, 5));

    AABB_t const gSpaceAABB = { .min = transform * glm::vec4(m_box.min, 1.f),
                                .max = transform * glm::vec4(m_box.max, 1.f) };
    return gSpaceAABB;
}

void Player::onTick(F32_t deltaTime)
{
    // update bounding box
    glm::mat4      &transform = g_scene.getNodeBySid(m_sid)->absoluteTransform;
    glm::vec3 const displacement = displacementTick(deltaTime);
    transform                    = glm::translate(transform, displacement);
    m_worldObjPtr->ebox          = recomputeGlobalSpaceBB();

    // TODO remove when chunking
    m_lastDisplacement = displacement;

    auto const meshCenter = centroid(m_worldObjPtr->ebox);

    // TODO add effective movement
    Ray_t const ray{ .o = m_camera.position, .d = meshCenter - m_camera.position };
    Hit_t       hit;
    g_world.build();
    if (g_world.intersect(ray, 0, hit))
    {
        //if (hit.t <= 0.5F) { m_camera.position = oldPosition; }
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
    auto        &transform = g_scene.getNodeBySid(m_sid)->absoluteTransform;
    static F32_t yaw       = 0;
    static F32_t pitch     = 0;
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
glm::vec3 Player::getCentroid() const {
    return centroid(m_worldObjPtr->ebox);
}

} // namespace cge