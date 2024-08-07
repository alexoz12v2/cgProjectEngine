#pragma once

// TODO: Add logging
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Core/Utility.h"

#include <memory_resource>

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

class IModule
{
  public:
    IModule(Sid_t id);
    virtual ~IModule() = default;

  public:
    virtual void onInit(ModuleInitParams params);
    virtual void onTick(float deltaTime) = 0;

    bool  taggedForDestruction() const;
    Sid_t moduleSwitched() const;
    void  resetSwitchModule();

  protected:
    void tagForDestruction();
    void switchToModule(Sid_t moduleSid);
    bool initializedOnce() const;

  private:
    Sid_t m_nextModule           = nullSid;
    bool  m_taggedForDestruction = false;
    Sid_t m_id                   = nullSid;
};

std::pmr::unsynchronized_pool_resource *getMemoryPool();
std::pmr::monotonic_buffer_resource    *getscratchBuffer();
} // namespace cge
