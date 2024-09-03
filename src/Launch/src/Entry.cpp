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

inline U32_t constexpr timeWindowPower = 2u;
inline U32_t constexpr timeWindowSize  = 1u << timeWindowPower;
inline U32_t constexpr timeWindowMask  = (timeWindowSize - 1);

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
        getModuleMap().emplace(
          std::piecewise_construct,
          std::forward_as_tuple(CGE_SID(moduleStr)),
          std::forward_as_tuple(nullptr, f));
    }

} // namespace detail
} // namespace cge

static void setMXCSR_DAZ_FTZ()
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
}

using namespace cge;

class MainTimer
{
    static U32_t constexpr timeWindowPower = 2u;
    static U32_t constexpr timeWindowSize  = 1u << timeWindowPower;
    static U32_t constexpr timeWindowMask  = (timeWindowSize - 1);
  public:
    MainTimer() {
        std::fill_n(timeWindow.data(), timeWindowSize, timeUnitsIn60FPS);
        reset();
    }

    void reset() {
        startTime = hiResTimer();
    }

    U32_t elapsedTime() {
        U32_t measuredElapsedTime = elapsedTimeUnits(hiResTimer(), startTime);
#if defined(CGE_DEBUG)
        // if the elapsed time is too big, we must have had a breakpoint. Set
        // elapsed time to ideal
        if (measuredElapsedTime > timeUnit32)
        {
            measuredElapsedTime = timeUnitsIn60FPS;
        }
#endif
        timeWindow[timeWindowIndex] = measuredElapsedTime;
        ++timeWindowIndex;
        timeWindowIndex &= timeWindowMask;

        measuredElapsedTime = 0;
        for (U32_t i = 0; i != timeWindowSize; ++i)
        {
            measuredElapsedTime += timeWindow[i];
        }

        // equal to measuredElapsedTime / timeWindowSize
        measuredElapsedTime >>= timeWindowPower;
        return measuredElapsedTime;
    }

  private:
    Array<U32_t, timeWindowSize> timeWindow{};
    U64_t startTime = 0;
    U32_t timeWindowIndex = 0;
};

I32_t main(I32_t argc, Char8_t **argv)
{
    setMXCSR_DAZ_FTZ();
    g_eventQueue.init();
    MainTimer mainTimer;
    U32_t elapsedTime = timeUnitsIn60FPS;
    F32_t elapsedTimeF = oneOver60FPS;

    WindowSpec_t windowSpec{
        .title = "window", .width = 600, .height = 480
    };
    Window_s window;
    window.init(windowSpec);

    g_focusedWindow.setFocusedWindow(&window);

    // construct startup module
    getModuleMap().at(g_startupModule).ctor();
    printf("past pre initialization\n");

    ModuleInitParams const params{};
    getModuleMap().at(g_startupModule).pModule->onInit(params);

    g_renderer.init();
    g_renderer2D.init();
    window.emitFramebufferSize();
    g_eventQueue.dispatch();

    printf("past initialization\n");

    while (
      !window.shouldClose()
      && !getModuleMap().at(g_startupModule).pModule->taggedForDestruction())
    {
        mainTimer.reset();

        if (Sid_t const sid =
              getModuleMap().at(g_startupModule).pModule->moduleSwitched();
            sid != nullSid)
        {
            if (getModuleMap().contains(sid))
            { // destroy current module and set it to nullptr
                delete getModuleMap().at(g_startupModule).pModule;
                getModuleMap().at(g_startupModule).pModule = nullptr;

                g_startupModule = sid;
                getModuleMap().at(g_startupModule).ctor();
                getModuleMap().at(g_startupModule).pModule->onInit(params);
                // Doesn't make sense, but needed, otherwise viewport breaks
                // when changing modules
                window.emitFramebufferSize();
                g_eventQueue.dispatch();
            }
            else
            { //
                getModuleMap().at(g_startupModule).pModule->resetSwitchModule();
            }
        }

        // Do stuff...
        g_renderer.clear();
        getModuleMap().at(g_startupModule).pModule->onTick(elapsedTimeF);

        // swap buffers and poll events (and queue them)
        window.swapBuffers();
        window.pollEvents(0);

        // dispatch events
        g_eventQueue.dispatch();

        // Update timers
        elapsedTime  = mainTimer.elapsedTime();
        elapsedTimeF = (F32_t)elapsedTime / timeUnit32;
        printf("[main] deltaTime: %f\n", elapsedTimeF);
    }

    for (auto &pair : getModuleMap())
    { // if the pointer is nullptr delete is nop
        delete pair.second.pModule;
    }
}
