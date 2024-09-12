#include "Entry.h"

#include "Core/Alloc.h"
#include "Core/Containers.h"
#include "Core/Event.h"
#include "Core/Module.h"
#include "Core/TimeUtils.h"
#include "Core/Type.h"
#include "Render/Renderer.h"
#include "Render/Renderer2d.h"
#include "Render/Window.h"

#include <chrono>
#include <cstdio>

namespace cge
{

ModuleMap &getModuleMap()
{
    static ModuleMap modules{ getMemoryPool() };
    return modules;
}
Sid_t g_startupModule;

namespace detail
{
    void addModule(Char8_t const *moduleStr, std::function<void()> const &f)
    {
        getModuleMap().try_emplace(CGE_SID(moduleStr), f);
    }

} // namespace detail
} // namespace cge

static void setMXCSR_DAZ_FTZ()
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
}

using namespace cge;

#if 1
class MainTimer
{
    static constexpr U32_t timeWindowPower    = 2u;
    static constexpr U32_t timeWindowCapacity = 1u << timeWindowPower;
    static constexpr U32_t timeWindowMask     = (timeWindowCapacity - 1);
    static constexpr U64_t timeUnitsPerSecond = timeUnit64;

  public:
    MainTimer()
    {
        std::fill_n(m_timeWindow.data(), timeWindowCapacity, timeUnitsIn60FPS);
        reset();
    }

    void reset()
    {
        m_startTime = getCurrentTime();
    }

    // Returns elapsed time in units of 1/3000 seconds
    U64_t elapsedTime()
    {
        TimePoint currentTime         = getCurrentTime();
        U64_t     measuredElapsedTime = elapsedTimeUnits(currentTime, m_startTime);

        m_startTime = currentTime; // Update start time for next calculation

#if defined(CGE_DEBUG)
        // Handle cases where a debugger causes a large pause
        if (measuredElapsedTime > timeUnit32)
        {
            measuredElapsedTime = timeUnitsIn60FPS;
        }
#endif
#undef min
        // Store the measured time in the circular buffer
        m_timeWindow[m_timeWindowIndex] = measuredElapsedTime;
        ++m_timeWindowIndex;
        m_timeWindowIndex &= timeWindowMask;
        m_timeWindowSize = glm::min(m_timeWindowSize + 1, timeWindowCapacity);

        // Compute the average elapsed time
        U64_t totalElapsedTime = 0;
        for (U32_t i = 0; i != m_timeWindowSize; ++i)
        {
            totalElapsedTime += m_timeWindow[i];
        }
        U64_t averagedElapsedTime = totalElapsedTime / m_timeWindowSize;

        return averagedElapsedTime;
    }

  private:
    using Clock     = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    TimePoint                             m_startTime;
    std::array<U64_t, timeWindowCapacity> m_timeWindow{};
    U32_t                                 m_timeWindowIndex = 0;
    U32_t                                 m_timeWindowSize  = 0;

    // Returns the current time in nanoseconds
    TimePoint getCurrentTime() const
    {
        return Clock::now();
    }

    // Convert elapsed time from nanoseconds to 1/3000 seconds units
    U64_t elapsedTimeUnits(const TimePoint &endTime, const TimePoint &startTime) const
    {
        auto deltaTimeNano = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
        // Convert nanoseconds to time units (1/3000 seconds)
        return (deltaTimeNano * timeUnitsPerSecond) / 1'000'000'000ULL;
    }
};
#else
class MainTimer
{
    static U32_t constexpr timeWindowPower    = 2u;
    static U32_t constexpr timeWindowCapacity = 1u << timeWindowPower;
    static U32_t constexpr timeWindowMask     = (timeWindowCapacity - 1);

  public:
    MainTimer()
    {
        std::fill_n(m_timeWindow.data(), timeWindowCapacity, timeUnitsIn60FPS);
        reset();
    }

    void reset()
    {
        m_startTime = hiResTimer();
    }

    U32_t elapsedTime()
    {
        U32_t measuredElapsedTime = elapsedTimeUnits(hiResTimer(), m_startTime);
#if defined(CGE_DEBUG)
        // if the elapsed time is too big, we must have had a breakpoint. Set
        // elapsed time to ideal
        if (measuredElapsedTime > timeUnit32)
        {
            measuredElapsedTime = timeUnitsIn60FPS;
        }
#endif
#undef min
        m_timeWindow[m_timeWindowIndex] = measuredElapsedTime;
        ++m_timeWindowIndex;
        m_timeWindowIndex &= timeWindowMask;
        m_timeWindowSize = glm::min(m_timeWindowSize + 1, timeWindowCapacity);

        measuredElapsedTime = 0;
        for (U32_t i = 0; i != m_timeWindowSize; ++i)
        {
            measuredElapsedTime += m_timeWindow[i];
        }

        measuredElapsedTime /= m_timeWindowSize;
        return measuredElapsedTime;
    }

  private:
    Array<U32_t, timeWindowCapacity> m_timeWindow{};
    U64_t                            m_startTime       = 0;
    U32_t                            m_timeWindowIndex = 0;
    U32_t                            m_timeWindowSize  = 0;
};
#endif
I32_t main(I32_t argc, Char8_t **argv)
{
    setMXCSR_DAZ_FTZ();
    g_eventQueue.init();
    MainTimer mainTimer;
    U64_t     elapsedTime = timeUnitsIn60FPS;

    WindowSpec_t windowSpec{ .title = "window", .width = 600, .height = 480 };
    Window_s     window;
    window.init(windowSpec);

    g_focusedWindow.setFocusedWindow(&window);

    g_renderer.init();
    g_renderer2D.init();
    window.emitFramebufferSize();

    // construct startup module
    getModuleMap().at(g_startupModule).ctor();

    UntypedData128 const params{};
    getModuleMap().at(g_startupModule).pModule->onInit();

    g_eventQueue.dispatch();

    while (!window.shouldClose() && !getModuleMap().at(g_startupModule).pModule->taggedForDestruction())
    {
        mainTimer.reset();

        if (Sid_t const sid = getModuleMap().at(g_startupModule).pModule->moduleSwitched(); sid != nullSid)
        {
            if (getModuleMap().contains(sid))
            { // destroy current module and set it to nullptr
                delete getModuleMap().at(g_startupModule).pModule;
                getModuleMap().at(g_startupModule).pModule = nullptr;

                g_startupModule = sid;
                getModuleMap().at(g_startupModule).ctor();
                getModuleMap().at(g_startupModule).pModule->onInit();
                // Doesn't make sense, but needed, otherwise viewport breaks
                // when changing modules
                window.emitFramebufferSize();
                g_eventQueue.dispatch();
            }
            else
            {
                getModuleMap().at(g_startupModule).pModule->resetSwitchModule();
            }
        }

        // Do stuff...
        g_renderer.clear();
        getModuleMap().at(g_startupModule).pModule->onTick(elapsedTime);

        // swap buffers and poll events (and queue them)
        window.swapBuffers();
        window.pollEvents(0);

        // dispatch events
        g_eventQueue.dispatch();

        // Update timers
        elapsedTime = mainTimer.elapsedTime();
    }

    for (auto &[sid, moduleCtorPair] : getModuleMap())
    { // if the pointer is nullptr delete is nop
        delete moduleCtorPair.pModule;
    }
}
