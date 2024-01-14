#include "Entry.h"

#include "Core/Alloc.h"
#include "Core/Containers.h"
#include "Core/Event.h"
#include "Core/Module.h"
#include "Core/TimeUtils.h"
#include "Core/Type.h"
#include "Render/Window.h"

#include <cstdio>
#include <GLFW/glfw3.h>
namespace cge
{


inline U32_t constexpr timeWindowPower = 2u;
inline U32_t constexpr timeWindowSize  = 1u << timeWindowPower;

// change in ISceneModule
IModule               *g_startupModule = nullptr;
std::function<void()> *g_constructStartupModule;
Memory_s               g_memory;
Pool_t                 g_pool;
DoubleBuffer_t         g_doublebuffer;

EErr_t g_initializerStatus;

namespace detail
{

    EErr_t initMemory()
    {
        static U64_t constexpr memoryPower     = 30; // 2^30 = 1 GiB
        static U64_t constexpr memoryBytes     = 1 << memoryPower;
        static U64_t constexpr memQuarterBytes = memoryBytes >> 2;

        // allocate system memory
        g_initializerStatus = g_memory.init(memoryBytes, 16);
        if (g_initializerStatus != EErr_t::eSuccess)
        {
            return g_initializerStatus;
        }
        printf("allocated memory\n");

        // initialize pool
        g_pool.init(g_memory.atOffset(0), memQuarterBytes);
        printf("created pool\n");

        // initialize memory structures
        g_doublebuffer.init(
          g_memory.atOffset(memQuarterBytes), memQuarterBytes);
        printf("created double buffer\n");

        return g_initializerStatus;
    }

} // namespace detail

struct AppStatus_t
{
    [[nodiscard]] bool all() const { return windowStayOpen; }

    bool windowStayOpen;
};

bool appShouldRun(AppStatus_t const &status) { return status.all(); }

void *windowPointer = nullptr;

void enableCursor()
{
    assert(windowPointer);
    auto *window = (GLFWwindow *)windowPointer;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void disableCursor()
{
    assert(windowPointer);
    auto *window = (GLFWwindow *)windowPointer;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

} // namespace cge

template<std::integral T>
constexpr T min(T a, T b)
{
    return a < b ? a : b;
}
using namespace cge;
I32_t main(I32_t argc, Char8_t **argv)
{
    Array<U32_t, timeWindowSize> timeWindow;
    std::fill_n(timeWindow.data(), timeWindowSize, timeUnitsIn60FPS);

    U32_t timeWindowIndex = 0;
    U32_t elapsedTime     = timeUnitsIn60FPS;
    F32_t elapsedTimeF    = oneOver60FPS;

    WindowSpec_t windowSpec{
        .title = "window", .width = 600, .height = 480
    }; // TODO: read startup config from yaml
    Window_s    window;
    AppStatus_t appStatus{ true };

    window.init(windowSpec);
    windowPointer = window.internal();
    printf("past pre initialization\n");
    (*g_constructStartupModule)();

    ModuleInitParams const params{};
    g_startupModule->onInit(params);

    window.emitFramebufferSize();

    printf("past initialization\n");

    while (appShouldRun(appStatus))
    {
        U64_t startTime = hiResTimer();

        // Do stuff...
        g_startupModule->onTick(elapsedTimeF);

        U64_t endTime = hiResTimer();

        U32_t measuredElapsedTime = elapsedTimeUnits(endTime, startTime);
#if defined(CGE_DEBUG)
        // if the elapsed time is too big, we must have had a breakpoint.  Set
        // elapsed time to ideal
        if (measuredElapsedTime > timeUnit32)
        {
            measuredElapsedTime = timeUnitsIn60FPS;
        }
#endif
        // perform time average
        timeWindowIndex             = (timeWindowIndex + 1) >> timeWindowPower;
        timeWindow[timeWindowIndex] = measuredElapsedTime;

        measuredElapsedTime = 0;
        for (U32_t i = 0; i != timeWindowSize; ++i)
        {
            measuredElapsedTime += timeWindow[i];
        }
        measuredElapsedTime >>= timeWindowPower;

        elapsedTime  = measuredElapsedTime;
        elapsedTimeF = (F32_t)elapsedTime / timeUnit32;

        // swap buffers and poll events (and queue them)
        window.setDeltaTime(elapsedTimeF);
        window.swapBuffers();
        window.pollEvents(min(timeUnitsIn60FPS - elapsedTime, 0U));

        // dispatch events
        g_eventQueue.dispatch();

        // if grows move into dedicated function
        appStatus.windowStayOpen = !window.shouldClose();
    }

    delete g_startupModule;
    delete g_constructStartupModule;
    return 0;
}
