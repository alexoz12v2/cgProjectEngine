#pragma once

#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Core/Utility.h"
#include <gsl/pointers>

#include <memory_resource>
#include <unordered_map>

namespace cge
{

union UntypedData128
{
    gsl::owner<Byte_t *> p[2]; // owner
    I64_t                i64[2];
    I32_t                i32[4];
    I16_t                i16[8];
    I8_t                 i8[16];
};
static_assert(sizeof(UntypedData128) == 16);

class IModule
{
  public:
    IModule(Sid_t id);
    virtual ~IModule() = default;

  public:
    virtual void onInit();
    virtual void onTick(U64_t deltaTime) = 0;

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

class GlobalStore
{
  public:
    GlobalStore();

    // the caller is responsible to eventually free untypedData.p pointers
    UntypedData128 consume(Sid_t const &sid);
    void put(Sid_t const &sid, UntypedData128 const &data);

  private:
    std::pmr::unordered_map<Sid_t, UntypedData128> m_map;
};

std::pmr::unsynchronized_pool_resource *getMemoryPool();
std::pmr::monotonic_buffer_resource    *getscratchBuffer();
extern GlobalStore g_globalStore;

} // namespace cge
