#include "Event.h"

#include <alloca.h>
#include <cstring>
#include <tuple>

namespace cge
{

EventQueue_t g_eventQueue;

EErr_t EventQueue_t::init()
{
    m_multimap.reserve(256);
    return EErr_t::eSuccess;
}

EErr_t EventQueue_t::dispatch()
{
    for (; !m_queue.empty(); m_queue.pop())
    {
        auto const& [event, eventData] = m_queue.front();
        auto const range               = m_multimap.equal_range(event);
        for (auto it = range.first; it != range.second; ++it)
        { //
            it->second.listenerFunc(eventData, it->second.listenerData);
        }
    }
    return EErr_t::eSuccess;
}

EErr_t EventQueue_t::emit(Event_t event, EventArg_t eventData)
{
    m_queue.emplace(event, eventData);
    return EErr_t::eSuccess;
}

static Sid_t bytesSid(EventArg_t obj)
{
    static U64_t constexpr size = sizeof(EventArg_t);
    Char8_t* memory = static_cast<decltype(memory)>(alloca(size + 1));
    std::memcpy(memory, &obj, size);
    memory[size] = '\0';
    return CGE_SID(memory);
}

EventDataSidPair EventQueue_t::addListener(
  Event_t     event,
  EventFunc_t listener,
  EventArg_t  listenerData)
{
    m_multimap.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(event),
      std::forward_as_tuple(listener, listenerData));
    return { event, bytesSid(listenerData) };
}

void EventQueue_t::removeListener(EventDataSidPair const &pair)
{
    auto const range = m_multimap.equal_range(pair.ev);
    for (auto it = range.first; it != range.second; ++it)
    {
        Sid_t const sid = bytesSid(it->second.listenerData);
        if (pair.dataSid == sid)
        { //
            m_multimap.erase(it);
            break;
        }
    }
}

} // namespace cge
