#pragma once

#include "Core/Event.h"
#include "Core/StringUtils.h"

#include <glm/ext/vector_float3.hpp>

namespace cge
{

inline F32_t constexpr CLIPDISTANCE   = 5.f;
inline F32_t constexpr RENDERDISTANCE = 500.f;
inline F32_t constexpr FOV            = 90.f;

inline Event_t constexpr evGameOver{ .m_id = "GAME OVER"_sid };
inline Event_t constexpr evShoot{ .m_id = "SHOOT"_sid };
inline Event_t constexpr evMagnetAcquired{ .m_id = "MAGNET POWERUP"_sid };
inline Event_t constexpr evSpeedAcquired{ .m_id = "SPEED POWERUP"_sid };

template<typename T>
concept GameOverListener = requires(T t, U64_t i) { t.onGameOver(i); };

template<typename T>
concept ShootListener = requires(T t, Ray r) { t.onShoot(r); };

template<typename T>
concept MagnetAcquiredListener = requires(T t) { t.onMagnetAcquired(); };

template<typename T>
concept SpeedAcquiredListener = requires(T t) { t.onSpeedAcquired(); };

template<GameOverListener T> void gameOverCallback(EventArg_t eventData, EventArg_t listenerData)
{
    auto self = (T *)listenerData.idata.p;
    self->onGameOver(eventData.idata.u64);
}

template<ShootListener T> void shootCallback(EventArg_t eventData, EventArg_t listenerData)
{
    auto      self = (T *)listenerData.idata.p;
    glm::vec3 orig{ eventData.fdata.f32[0], eventData.fdata.f32[1], eventData.fdata.f32[2] };
    glm::vec3 dir{ eventData.idata.i16[0], eventData.idata.i16[1], eventData.idata.i16[2] };
    Ray       ray{ orig, dir };
    self->onShoot(ray);
}

template<MagnetAcquiredListener T> void magnetAcquiredCallback(EventArg_t evData, EventArg_t listenerData)
{
    auto self = (T *)listenerData.idata.p;
    self->onMagnetAcquired();
}

template<SpeedAcquiredListener T> void speedAcquiredCallback(EventArg_t evData, EventArg_t listenerData)
{
    auto *self = (T *)listenerData.idata.p;
    self->onSpeedAcquired();
}

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
    static U32_t constexpr len = N;
    [[nodiscard]] constexpr Char8_t const *cStr() const { return buf; }
    Char8_t                  buf[N];
};

inline FixedString<numDigits(std::numeric_limits<U64_t>::max()) + 1> strFromIntegral(U64_t num)
{
    static U32_t constexpr maxLen = numDigits(std::numeric_limits<U64_t>::max());
    FixedString<maxLen + 1> res{};
    sprintf(res.buf, "%zu\0", num);

    return res;
}

// the first string includes already the nul terminator
template<FixedString str>
inline FixedString<str.len + 2 + numDigits(std::numeric_limits<U64_t>::max())> fixedStringWithNumber(U64_t num)
{
    static U32_t constexpr len = str.len + 2 + numDigits(std::numeric_limits<U64_t>::max());
    FixedString<len> res{};
    sprintf(res.buf, str.cStr());
    sprintf(res.buf + str.len - 1, ": %zu\0", num);

    return res;
}

} // namespace cge
