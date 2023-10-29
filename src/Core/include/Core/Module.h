#pragma once
// MIT License. Copyright (c) 2023

// TODO: Add logging
#include "Type.h"

union ModuleInitParams {
    Byte_t *p[2];
    I64_t i64[2];
    I32_t i32[4];
    I16_t i16[8];
    I8_t i8[16];
};
static_assert(sizeof(ModuleInitParams) == 16);

/** @class IModule
 * @brief
 */
class IModule
{
  public:
    /** @fn onInit
     * @brief callback function called after all modules have been initialized
     * @param params initialization parameters. Contains arbitrary data
     */
    virtual void onInit(ModuleInitParams params) = 0;

    virtual ~IModule() = default;
};
