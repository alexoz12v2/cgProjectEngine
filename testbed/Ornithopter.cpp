#include "Ornithopter.h"

#include "Core/TimeUtils.h"
#include "Resource/HandleTable.h"
#include "Resource/Rendering/cgeScene.h"
#include "SoundEngine.h"
#include "Utils.h"

#include <cassert>
#include <irrKlang/ik_ISoundSource.h>

namespace cge
{
// ornithopter constants
inline F32_t constexpr halfLimitAngle       = 5.f * glm::pi<F32_t>() / 180.f;
inline F32_t constexpr rotationFrequency    = 100.f * glm::pi<F32_t>() * 0.5f;
inline F32_t constexpr negativeRotationSkew = 0.2f;

Ornithopter::Ornithopter(OrnithopterSpec const &spec)
{
    U32_t i = 0;
    for (Sid_t const &sid : { spec.body, spec.wingUpR, spec.wingUpL, spec.wingBottomR, spec.wingBottomL })
    {
        m_sids.arr[i++] = g_scene.addNode(sid);
    }
}

Ornithopter::~Ornithopter()
{
    for (irrklang::ISoundSource *source : { m_swishSoundSource, m_gunSoundSource, m_helicopterSoundSource })
    {
        if (source)
        {
            g_soundEngine()->stopAllSoundsOfSoundSource(source);
            g_soundEngine()->removeSoundSource(source);
        }
    }

    for (Sid_t const &sid : m_sids.arr)
    {
        g_scene.removeNode(sid);
    }
}

void Ornithopter::init(glm::mat4 const &initialTransform)
{
    m_swishSoundSource      = g_soundEngine()->addSoundSourceFromFile("../assets/swish.mp3");
    m_gunSoundSource        = g_soundEngine()->addSoundSourceFromFile("../assets/gun-shot.mp3");
    m_helicopterSoundSource = g_soundEngine()->addSoundSourceFromFile("../assets/helicopter-blades.mp3");
    assert(m_swishSoundSource && m_gunSoundSource && m_helicopterSoundSource);
    g_soundEngine()->play2D(m_helicopterSoundSource, true);

    for (Sid_t const &sid : m_sids.arr)
    {
        g_scene.getNodeBySid(sid).setTransform(initialTransform);
    }
}

void Ornithopter::onTick(U64_t deltaTime, OnTickTs const &transforms)
{
    // if the order in the struct of m_sids is changed, this needs to change too
    static F32_t constexpr rotationOrientations[]{ 1.f, -1.f, -1.f, 1.f };
    m_elapsedTime += deltaTime;
    for (Sid_t const &sid : m_sids.arr)
    {
        g_scene.getNodeBySid(sid).setTransform(glm::mat4(1.f));
    }

    // Wings rotation
    U32_t index = 0;
    for (auto const &sid : m_sids.arr)
    {
        if (sid == m_sids.s.body)
        {
            continue;
        }

        SceneNode_s &node = g_scene.getNodeBySid(sid);
        F32_t radians = halfLimitAngle * glm::sin(rotationFrequency * m_elapsedTime / timeUnit64);
        if (radians < 0.f)
        {
            radians *= negativeRotationSkew;
        }
        radians *= rotationOrientations[index];
        node.rotate(radians, glm::vec3(0.f, -1.f, 0.f));

        ++index;
    }

    // TODO: shifting animation?
    for (Sid_t const &sid : m_sids.arr)
    {
        g_scene.getNodeBySid(sid).rightMul(transforms.playerTransform * glm::rotate(glm::mat4(1.f), glm::half_pi<F32_t>(), glm::vec3(1.f, 0.f, 0.f)));
        g_scene.getNodeBySid(sid).transform(transforms.cameraTransform * transforms.playerTranslate);
    }
}

AABB Ornithopter::bodyBoundingBox() const
{
    SceneNode_s const &node          = g_scene.getNodeBySid(m_sids.s.body);
    AABB const         modelSpaceBox = g_handleTable.getMesh(node.getSid()).box;
    return globalSpaceBB(node, modelSpaceBox);
}

void Ornithopter::playGun()
{
    g_soundEngine()->play2D(m_gunSoundSource);
}
void Ornithopter::playSwish()
{
    g_soundEngine()->play2D(m_swishSoundSource);
}
void Ornithopter::stopSwish()
{
    g_soundEngine()->stopAllSoundsOfSoundSource(m_swishSoundSource);
}

} // namespace cge