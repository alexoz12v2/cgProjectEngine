#pragma once

#include "Core/Containers.h"
#include "Core/Type.h"

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
    EventComplexArg_t *c;
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

struct EventArgs_t
{
    EventIntArg_t idata[2];
    V128_t        fdata[2];
};
