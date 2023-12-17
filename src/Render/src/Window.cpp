#include "Window.h"

#include "Core/Event.h"
#include "Core/Events.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

#include <thread>

namespace cge
{

Window_s::~Window_s()
{
    if (handle)
    {
        glfwDestroyWindow(handle);
        glfwTerminate();
    }
}

EErr_t Window_s::init(WindowSpec_t const& spec)
{
    if (handle) { return EErr_t::eInvalid; }
    if (!glfwInit()) { return EErr_t::eCreationFailure; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    handle =
      glfwCreateWindow(spec.width, spec.height, spec.title, nullptr, nullptr);
    if (!handle)
    {
        glfwTerminate();
        return EErr_t::eCreationFailure;
    }

    glfwMakeContextCurrent(handle);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        glfwTerminate();
        return EErr_t::eCreationFailure;
    }

    if (!GLAD_GL_VERSION_4_6)
    {
        printf("OpenGL 4.1 is not supported");
        glfwTerminate();
        return EErr_t::eCreationFailure;
    }
    if (GLAD_GL_ARB_separate_shader_objects)
    {
        printf("ARB_separate_shader_objects is supported");
    }

    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    // set this object as user poI32_ter for callbacks
    glfwSetWindowUserPointer(handle, this);

    // set cursor mode
    glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // set member function callbacks
    glfwSetKeyCallback(handle, &Window_s::keyCallback);
    glfwSetMouseButtonCallback(handle, &Window_s::mouseButtonCallback);
    glfwSetCursorPosCallback(handle, &Window_s::cursorPositionCallback);
    glfwSetErrorCallback(&Window_s::errorCallback);
    glfwSetFramebufferSizeCallback(handle, &Window_s::framebufferSizeCallback);

    // enable V-Sync (TODO configurable?)
    glfwSwapInterval(1);

    glViewport(0, 0, spec.width, spec.height);

    return EErr_t::eSuccess;
}

bool Window_s::shouldClose() { return glfwWindowShouldClose(handle); }

void Window_s::keyCallback(
  GLFWwindow* window,
  I32_t       key,
  I32_t       scancode,
  I32_t       action,
  I32_t       mods)
{
    // Access the Window instance from the user poI32_ter
    auto instance = static_cast<Window_s*>(glfwGetWindowUserPointer(window));
    if (instance)
    {
        // Call member function
        instance->onKey(key, scancode, action, mods);
    }
}

void Window_s::errorCallback(I32_t error, const char* description)
{
    // TODO: handle errors (ie report and crash)
}

void Window_s::cursorPositionCallback(
  GLFWwindow* window,
  F64_t       xpos,
  F64_t       ypos)
{
    auto instance = static_cast<Window_s*>(glfwGetWindowUserPointer(window));
    if (instance) { instance->onCursorMovement((F32_t)xpos, (F32_t)ypos); }
}

void Window_s::mouseButtonCallback(
  GLFWwindow* window,
  I32_t       button,
  I32_t       action,
  I32_t       mods)
{
    auto instance = static_cast<Window_s*>(glfwGetWindowUserPointer(window));
    if (instance) { instance->onMouseButton(button, action, mods); }
}

void Window_s::framebufferSizeCallback(
  GLFWwindow* window,
  I32_t       width,
  I32_t       height)
{
    auto instance = static_cast<Window_s*>(glfwGetWindowUserPointer(window));
    if (instance)
    {
        // Call member function
        instance->onFramebufferSize(width, height);
    }
}

// Member function callbacks
void Window_s::onKey(I32_t key, I32_t scancode, I32_t action, I32_t mods) const
{
    // Handle key events here (queued)
    EventArg_t keyPressedData{};
    keyPressedData.idata.i32[0] = key;
    keyPressedData.idata.i32[1] = action;
    keyPressedData.fdata.f32[0] = deltaTime;
    g_eventQueue.addEvent(evKeyPressed, keyPressedData);
}

void Window_s::onMouseButton(I32_t button, I32_t action, I32_t mods) const
{
    // Handle button events here (queued)
    EventArg_t keyPressedData{};
    keyPressedData.idata.i32[0] = button;
    keyPressedData.idata.i32[1] = action;
    keyPressedData.fdata.f32[0] = deltaTime;
    g_eventQueue.addEvent(evMouseButtonPressed, keyPressedData);
}

void Window_s::onCursorMovement(F32_t xpos, F32_t ypos) const
{
    // Handle mouse events here (queued)
    EventArg_t cursorCoords{};
    cursorCoords.fdata.f32[0] = xpos;
    cursorCoords.fdata.f32[1] = ypos;
    g_eventQueue.addEvent(evMouseMoved, cursorCoords);
}

void Window_s::onFramebufferSize(I32_t width, I32_t height) const
{
    // Handle framebuffer size changes here
    glViewport(0, 0, width, height);
    EventArg_t framebufferSize{};
    framebufferSize.idata.i32[0] = width;
    framebufferSize.idata.i32[1] = height;
    g_eventQueue.addEvent(evFramebufferSize, framebufferSize);
}

void Window_s::swapBuffers() { glfwSwapBuffers(handle); }

void Window_s::pollEvents(I32_t waitMillis) const
{
    glfwPollEvents();

    // delay to reduce CPU usage if our target of 60 FPS was more than reached,
    // and we need to wait some excess time
    std::this_thread::sleep_for(std::chrono::milliseconds(waitMillis));
}

void Window_s::emitFramebufferSize() const
{
    I32_t width;
    I32_t height;
    glfwGetFramebufferSize(handle, &width, &height);

    onFramebufferSize(width, height);
}

} // namespace cge
