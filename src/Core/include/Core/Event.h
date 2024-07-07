#pragma once

#include "Core/Containers.h"
#include "Core/Type.h"
#include "StringUtils.h"

#include <queue>

namespace cge
{

inline U32_t constexpr EVENT_MAX_COMPLEX_ARGS = 8;
struct EventComplexArg_t
{
    struct Pair_t
    {
        U64_t sid;
        U64_t value;
    };
    Array<Pair_t, EVENT_MAX_COMPLEX_ARGS> args;
};

union EventIntArg_t
{
    EventComplexArg_t *c; /// @warning takes ownership
    Byte_t            *p;
    U64_t              u64;
    I64_t              i64;
    U32_t              u32[2];
    I32_t              i32[2];
    U16_t              u16[4];
    I16_t              i16[4];
    I8_t               i8[8];
};
static_assert(sizeof(EventIntArg_t) == 8 && alignof(EventIntArg_t) == 8);

union EventFloatArg_t
{
    V128_t v;
    F32_t  f32[4];
    F64_t  f64[2];
};
static_assert(sizeof(V128_t));

struct EventArg_t
{
    EventIntArg_t   idata;
    EventFloatArg_t fdata;
};

using EventFunc_t = void (*)(EventArg_t eventData, EventArg_t listenerData);

struct Event_t
{
    Sid_t  m_id;
    size_t operator()(Event_t const &other) const
    {
        return std::hash<U64_t>()(other.m_id.id);
    }
};

inline B8_t operator==(Event_t const &a, Event_t const &b)
{
    return a.m_id == b.m_id;
}
} // namespace cge


template<> struct std::hash<cge::Event_t>
{
    std::size_t operator()(cge::Event_t s) const noexcept
    {
        std::size_t h = std::hash<cge::U64_t>{}(s.m_id.id);
        return h;
    }
};


namespace cge
{
class EventQueue_t
{
  public:
    EErr_t init();

    EErr_t dispatch();

    EErr_t addEvent(Event_t event, EventArg_t eventData);
    EErr_t
      addListener(Event_t event, EventFunc_t listener, EventArg_t listenerData);

  private:
    // TODO custom container
    std::unordered_map<Event_t, std::vector<std::pair<EventFunc_t, EventArg_t>>>
                                               m_map;
    std::queue<std::pair<Event_t, EventArg_t>> m_queue;
};

// TODO: when finish group all globals into singleton
extern EventQueue_t g_eventQueue;

template<typename T>
concept KeyListener = requires(T obj, I32_t x, I32_t y) { obj.onKey(x, y); };

template<KeyListener T>
void KeyCallback(EventArg_t eventData, EventArg_t listenerData)
{
    auto self = (T *)listenerData.idata.p;
    self->onKey(eventData.idata.i32[0], eventData.idata.i32[1]);
}

template<typename T>
concept MouseButtonListener =
  requires(T obj, I32_t x, I32_t y) { obj.onMouseButton(x, y); };

template<MouseButtonListener T>
void mouseButtonCallback(EventArg_t eventData, EventArg_t listenerData)
{
    auto self = (T *)listenerData.idata.p;
    self->onMouseButton(eventData.idata.i32[0], eventData.idata.i32[1]);
};

template<typename T>
concept MouseMovementListener =
  requires(T obj, F32_t x, F32_t y) { obj.onMouseMovement(x, y); };

template<MouseMovementListener T>
void mouseMovementCallback(EventArg_t eventData, EventArg_t listenerData)
{
    auto self = (T *)listenerData.idata.p;
    self->onMouseMovement(eventData.fdata.f32[0], eventData.fdata.f32[1]);
}

template<typename T>
concept FrameBufferSizeListener =
  requires(T obj, I32_t x, I32_t y) { obj.onFramebufferSize(x, y); };

template<FrameBufferSizeListener T>
void framebufferSizeCallback(EventArg_t eventData, EventArg_t listenerData)
{
    auto self = (T *)listenerData.idata.p;
    self->onFramebufferSize(eventData.idata.i32[0], eventData.idata.i32[1]);
}

} // namespace cge
