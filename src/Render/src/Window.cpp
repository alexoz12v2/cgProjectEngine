#include "Window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

#include <thread>

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

    // set this object as user pointer for callbacks
    glfwSetWindowUserPointer(handle, this);

    // set member function callbacks
    glfwSetKeyCallback(handle, &Window_s::keyCallback);
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
  int         key,
  int         scancode,
  int         action,
  int         mods)
{
    // Access the Window instance from the user pointer
    auto instance = static_cast<Window_s*>(glfwGetWindowUserPointer(window));
    if (instance)
    {
        // Call member function
        instance->onKey(key, scancode, action, mods);
    }
}

void Window_s::errorCallback(int error, const char* description)
{
    // TODO: handle errors (ie report and crash)
}

void Window_s::framebufferSizeCallback(
  GLFWwindow* window,
  int         width,
  int         height)
{
    // Access the Window instance from the user pointer
    auto instance = static_cast<Window_s*>(glfwGetWindowUserPointer(window));
    if (instance)
    {
        // Call member function
        instance->onFramebufferSize(width, height);
    }
}

// Member function callbacks
void Window_s::onKey(int key, int scancode, int action, int mods)
{
    // Handle key events here (queued)
}

void Window_s::onFramebufferSize(int width, int height)
{
    // Handle framebuffer size changes here
    glViewport(0, 0, width, height);
}

void Window_s::swapBuffers() { glfwSwapBuffers(handle); }

void Window_s::pollEvents(I32_t waitMillis)
{
    glfwPollEvents();

    // delay to reduce CPU usage if our target of 60 FPS was more than reached,
    // and we need to wait some excess time
    std::this_thread::sleep_for(std::chrono::milliseconds(waitMillis));
}
