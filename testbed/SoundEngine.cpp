#include "SoundEngine.h"

#include <cstdio>

namespace cge
{

SoundEngine g_soundEngine;

SoundEngine::SoundEngine() : m_soundEngine(irrklang::createIrrKlangDevice())
{
    if (!m_soundEngine)
    { //
        printf("[TestBed] error creating sound engine");
    }
}

SoundEngine::~SoundEngine() noexcept
{ //
    if (m_soundEngine)
        m_soundEngine->drop();
}

irrklang::ISoundEngine *SoundEngine::operator()()
{ //
    return m_soundEngine;
}

} // namespace cge