#pragma once

#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Core/Utility.h"

#include <irrKlang/ik_ISound.h>
#include <irrKlang/ik_ISoundSource.h>
#include <glm/ext/matrix_float4x4.hpp>
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
    explicit Ornithopter(OrnithopterSpec const &spec);
    ~Ornithopter();

    void init(glm::mat4 const &initialTransform);
    void onTick(U64_t deltaTime, glm::mat4 const &newBodyTransform);
    void playGun();
    void playSwish();
    void stopSwish();
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
        } s;
        Sid_t arr[5];
        static_assert(std::is_trivial_v<S> && sizeof(s) == sizeof(arr));
    } m_sids;

    // sound
    irrklang::ISoundSource *m_swishSoundSource{ nullptr };
    irrklang::ISoundSource *m_gunSoundSource{ nullptr };
    irrklang::ISoundSource *m_helicopterSoundSource{ nullptr };

    // misc
    U64_t m_elapsedTime{0ULL};
};

} // namespace cge
