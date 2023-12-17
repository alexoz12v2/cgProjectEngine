#include "Event.h"

namespace cge
{

EventQueue_t g_eventQueue;

EErr_t EventQueue_t::init()
{
    m_map.reserve(256);
    return EErr_t::eSuccess;
}

EErr_t EventQueue_t::dispatch()
{
    for (; !m_queue.empty(); m_queue.pop())
    {
        auto const& [event, eventData] = m_queue.front();
        if (auto const it = m_map.find(event); it != m_map.cend())
        {
            for (auto const& [func, listenerData] : it->second)
            {
                func(eventData, listenerData);
            }
        }
    }
    return EErr_t::eSuccess;
}

EErr_t EventQueue_t::addEvent(Event_t event, EventArg_t eventData)
{
    m_queue.emplace(event, eventData);
    m_map.try_emplace(event);
    return EErr_t::eSuccess;
}

EErr_t EventQueue_t::addListener(
  Event_t     event,
  EventFunc_t listener,
  EventArg_t  listenerData)
{
    auto it = m_map.find(event);
    if (it == m_map.cend())
    {
        auto pair = m_map.try_emplace(event);
        it        = pair.first;
    }
    if (it != m_map.cend())
    {
        it->second.emplace_back(listener, listenerData);
        return EErr_t::eSuccess;
    }
    return EErr_t::eInvalid;
}

} // namespace cge
