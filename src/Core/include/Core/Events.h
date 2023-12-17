#pragma once

#include "Core/Event.h"

namespace cge
{
inline Event_t constexpr evKeyPressed{ .m_id = "EVENT_KEY_PRESSED"_sid };
inline Event_t constexpr evMouseMoved{ .m_id = "EVENT_MOUSE_MOVED"_sid };
inline Event_t constexpr evMouseButtonPressed{
    .m_id = "EVENT_MOUSE_BUTTON_PRESSED"_sid
};
inline Event_t constexpr evFramebufferSize{ .m_id =
                                              "EVENT_FRAMEBUFFER_SIZE"_sid };
} // namespace cge
