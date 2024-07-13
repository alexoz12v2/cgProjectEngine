#pragma once

// TODO: Add logging
#include "Core/StringUtils.h"
#include "Core/Type.h"

namespace cge
{

union ModuleInitParams
{
    Byte_t *p[2];
    I64_t   i64[2];
    I32_t   i32[4];
    I16_t   i16[8];
    I8_t    i8[16];
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

    virtual void onTick(float deltaTime) = 0;

    virtual ~IModule() = default;

    bool  taggedForDestruction() const;
    Sid_t moduleSwitched() const;
    void  resetSwitchModule();

  protected:
    void tagForDestruction();
    void switchToModule(Sid_t moduleSid);

  private:
    bool  m_taggedForDestruction = false;
    Sid_t m_nextModule           = nullSid;
};
} // namespace cge
