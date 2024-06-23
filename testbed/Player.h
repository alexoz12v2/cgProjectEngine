#ifndef CGE_PLAYER_H
#define CGE_PLAYER_H

#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Entity/CollisionWorld.h"
#include "Render/Renderer.h"
#include "Resource/HandleTable.h"

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>

namespace cge
{

class Player
{
  public:
    static F32_t constexpr baseVelocity     = 100.F;
    static F32_t constexpr mouseSensitivity = 0.1F;

    void spawn(Camera_t const &, Sid_t);

    void onKey(I32_t key, I32_t action);
    void onMouseButton(I32_t key, I32_t action);
    void onMouseMovement(F32_t xPos, F32_t yPos);
    void onTick(F32_t deltaTime);

    [[nodiscard]] AABB_t    boundingBox() const;
    [[nodiscard]] glm::mat4 viewTransform() const;
    [[nodiscard]] Camera_t  getCamera() const;
    [[nodiscard]] glm::vec3 getCentroid() const;

    // TODO remove
    [[nodiscard]] glm::vec3 lastDisplacement() const;

  private:
    void      yawPitchRotate(F32_t yaw, F32_t pitch);
    AABB_t    recomputeGlobalSpaceBB() const;
    glm::vec3 displacementTick(F32_t deltaTime) const;

    // main components
    Sid_t                m_sid;
    HandleTable_s::Ref_s m_mesh = nullRef;
    Camera_t             m_camera{};

    // collision related
    AABB_t                      m_box;
    CollisionWorld_s::ObjHandle m_worldObjPtr;

    // movement related
    glm::ivec2 m_keyPressed{ 0, 0 }; // WS AD
    glm::vec2  m_lastCursorPosition{ -1.0F, -1.0F };
    B8_t       m_isCursorDisabled = false;
    glm::vec3  m_lastDisplacement{};
};

} // namespace cge

#endif // CGE_PLAYER_H
