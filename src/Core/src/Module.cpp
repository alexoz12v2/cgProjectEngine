#include "Module.h"

namespace cge
{

void IModule::tagForDestruction()
{ //
    m_taggedForDestruction = true;
}

bool IModule::taggedForDestruction() const { return m_taggedForDestruction; }

void IModule::switchToModule(Sid_t moduleSid)
{ //
    m_nextModule = moduleSid;
}

Sid_t IModule::moduleSwitched() const { return m_nextModule; }

void IModule::resetSwitchModule() { m_nextModule = nullSid; }

} // namespace cge
