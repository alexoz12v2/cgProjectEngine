#include "Ornithopter.h"

#include "Resource/HandleTable.h"
#include "Resource/Rendering/cgeScene.h"
#include "SoundEngine.h"
#include "Utils.h"

#include <cassert>
#include <irrKlang/ik_ISoundSource.h>

namespace cge
{

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

void Ornithopter::onTick(U64_t deltaTime, glm::mat4 const &newBodyTransform)
{
    m_elapsedTime += deltaTime;
    for (Sid_t const &sid : m_sids.arr)
    {
        g_scene.getNodeBySid(sid).setTransform(newBodyTransform);
    }

    // TODO: animate wings with sine function parametrised with m_elapsedTime
    // TODO: shifting animation?
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