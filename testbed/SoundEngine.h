#pragma once

#include <irrKlang/irrKlang.h>

namespace cge
{

class SoundEngine
{
  public:
    SoundEngine();
    SoundEngine(SoundEngine const &)                = delete;
    SoundEngine(SoundEngine &&) noexcept            = delete;
    SoundEngine &operator=(SoundEngine const &)     = delete;
    SoundEngine &operator=(SoundEngine &&) noexcept = delete;
    ~SoundEngine() noexcept;

    irrklang::ISoundEngine *operator()();

  private:
    irrklang::ISoundEngine *m_soundEngine{ nullptr };
};

extern SoundEngine g_soundEngine;

} // namespace cge