#pragma once

#include "Core/StringUtils.h"
#include "Resource/Ref.h"

namespace cge
{


struct WeakRef_t
{
    Sid_t sid;
    // not explicit purposefully
    inline WeakRef_t(EmptyRef_t) : sid(nullSid) {}
};
} // namespace cge
