#pragma once

#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Core/Utility.h"

#include <glm/ext/matrix_float4x4.hpp>
#include <irrKlang/ik_ISound.h>
#include <irrKlang/ik_ISoundSource.h>
#include <type_traits>

namespace cge
{

struct OrnithopterSpec
{
    Sid_t body;
    Sid_t wingUpR;
    Sid_t wingUpL;
    Sid_t wingBottomR;
    Sid_t wingBottomL;
};

class Ornithopter
{
  public:
    struct OnTickTs
    {
        glm::mat4 playerTransform;
        glm::mat4 playerTranslate;
        glm::mat4 cameraTransform;
    };

    explicit Ornithopter(OrnithopterSpec const &spec);
    ~Ornithopter();

    void               init(glm::mat4 const &initialTransform);
    void               onTick(U64_t deltaTime, OnTickTs const &transforms);
    void               playGun();
    void               playSwish();
    void               stopSwish();
    void               stopAllSounds();
    [[nodiscard]] AABB bodyBoundingBox() const;

  private:
    // scene sids
    union U
    {
        struct S
        {
            Sid_t body;
            Sid_t wingUpR;
            Sid_t wingUpL;
            Sid_t wingBottomR;
            Sid_t wingBottomL;
        };
        S     s;
        Sid_t arr[5];
        static_assert(std::is_trivial_v<S> && sizeof(s) == sizeof(arr));
    };
    U m_sids;

    // sound
    irrklang::ISoundSource *m_swishSoundSource{ nullptr };
    irrklang::ISoundSource *m_gunSoundSource{ nullptr };
    irrklang::ISoundSource *m_helicopterSoundSource{ nullptr };

    // misc
    U64_t m_elapsedTime{ 0ULL };
};

} // namespace cge
