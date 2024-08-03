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

FocusedWindow_s g_focusedWindow;


static void keyCallback(
  GLFWwindow* window,
  I32_t       key,
  I32_t       scancode,
  I32_t       action,
  I32_t       mods);

static void errorCallback(I32_t error, const char* description);
static void cursorPositionCallback(GLFWwindow* window, F64_t xpos, F64_t ypos);
static void mouseButtonCallback(
  GLFWwindow* window,
  I32_t       button,
  I32_t       action,
  I32_t       mods);

static void
  framebufferSizeCallback(GLFWwindow* window, I32_t width, I32_t height);


Window_s::~Window_s()
{
    if (m_handle)
    {
        glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(m_handle));
        glfwTerminate();
    }
}

EErr_t Window_s::init(WindowSpec_t const& spec)
{
    if (m_handle) { return EErr_t::eInvalid; }
    if (!glfwInit()) { return EErr_t::eCreationFailure; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_handle =
      glfwCreateWindow(spec.width, spec.height, spec.title, nullptr, nullptr);
    if (!m_handle)
    {
        glfwTerminate();
        return EErr_t::eCreationFailure;
    }

    glfwMakeContextCurrent(reinterpret_cast<GLFWwindow*>(m_handle));

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
        glfwSetInputMode(
          reinterpret_cast<GLFWwindow*>(m_handle),
          GLFW_RAW_MOUSE_MOTION,
          GLFW_TRUE);

    // set this object as user poI32_ter for callbacks
    glfwSetWindowUserPointer(reinterpret_cast<GLFWwindow*>(m_handle), this);

    // set cursor mode
    // #if !defined(CGE_DEBUG)
    //     glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // #endif

    // set member function callbacks
    glfwSetKeyCallback(reinterpret_cast<GLFWwindow*>(m_handle), &keyCallback);
    glfwSetMouseButtonCallback(
      reinterpret_cast<GLFWwindow*>(m_handle), &mouseButtonCallback);
    glfwSetCursorPosCallback(
      reinterpret_cast<GLFWwindow*>(m_handle), &cursorPositionCallback);
    glfwSetErrorCallback(&errorCallback);
    glfwSetFramebufferSizeCallback(
      reinterpret_cast<GLFWwindow*>(m_handle), &framebufferSizeCallback);

    // enable V-Sync (TODO configurable?)
    glfwSwapInterval(0);

    glViewport(0, 0, spec.width, spec.height);

    return EErr_t::eSuccess;
}

bool Window_s::shouldClose()
{
    return glfwWindowShouldClose(reinterpret_cast<GLFWwindow*>(m_handle));
}

void keyCallback(
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
        detail::WindowEventDispatcher(instance).onKey(
          key, scancode, action, mods);
    }
}

void errorCallback(I32_t error, const char* description)
{
    // TODO: handle errors (ie report and crash)
}

void cursorPositionCallback(GLFWwindow* window, F64_t xpos, F64_t ypos)
{
    auto instance = static_cast<Window_s*>(glfwGetWindowUserPointer(window));
    if (instance)
    {
        detail::WindowEventDispatcher(instance).onCursorMovement(
          (F32_t)xpos, (F32_t)ypos);
    }
}

void mouseButtonCallback(
  GLFWwindow* window,
  I32_t       button,
  I32_t       action,
  I32_t       mods)
{
    auto instance = static_cast<Window_s*>(glfwGetWindowUserPointer(window));
    if (instance)
    {
        detail::WindowEventDispatcher(instance).onMouseButton(
          button, action, mods);
    }
}

void framebufferSizeCallback(GLFWwindow* window, I32_t width, I32_t height)
{
    auto instance = static_cast<Window_s*>(glfwGetWindowUserPointer(window));
    if (instance)
    {
        // Call member function
        detail::WindowEventDispatcher(instance).onFramebufferSize(
          width, height);
    }
}

// Member function callbacks
void Window_s::onKey(I32_t key, I32_t scancode, I32_t action, I32_t mods) const
{
    // Handle key events here (queued)
    EventArg_t keyPressedData{};
    keyPressedData.idata.i32[0] = key;
    keyPressedData.idata.i32[1] = action;
    g_eventQueue.emit(evKeyPressed, keyPressedData);
}

void Window_s::onMouseButton(I32_t button, I32_t action, I32_t mods) const
{
    // Handle button events here (queued)
    EventArg_t keyPressedData{};
    keyPressedData.idata.i32[0] = button;
    keyPressedData.idata.i32[1] = action;
    g_eventQueue.emit(evMouseButtonPressed, keyPressedData);
}

void Window_s::onCursorMovement(F32_t xpos, F32_t ypos) const
{
    // Handle mouse events here (queued)
    EventArg_t cursorCoords{};
    cursorCoords.fdata.f32[0] = xpos;
    cursorCoords.fdata.f32[1] = ypos;
    g_eventQueue.emit(evMouseMoved, cursorCoords);
}

void Window_s::onFramebufferSize(I32_t width, I32_t height) const
{
    // Handle framebuffer size changes here
    EventArg_t m_framebufferSize{};
    m_framebufferSize.idata.i32[0] = width;
    m_framebufferSize.idata.i32[1] = height;
    g_eventQueue.emit(evFramebufferSize, m_framebufferSize);
}

void Window_s::swapBuffers()
{
    glfwSwapBuffers(reinterpret_cast<GLFWwindow*>(m_handle));
}

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
    glfwGetFramebufferSize(
      reinterpret_cast<GLFWwindow*>(m_handle), &width, &height);

    onFramebufferSize(width, height);
}

glm::ivec2 Window_s::getFramebufferSize() const
{
    glm::ivec2 res{ 0, 0 };
    glfwGetFramebufferSize(
      reinterpret_cast<GLFWwindow*>(m_handle), &res.x, &res.y);
    return res;
}

void* Window_s::internal() { return m_handle; }

void Window_s::enableCursor()
{
    glfwSetInputMode(
      reinterpret_cast<GLFWwindow*>(m_handle), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Window_s::disableCursor()
{
    glfwSetInputMode(
      reinterpret_cast<GLFWwindow*>(m_handle),
      GLFW_CURSOR,
      GLFW_CURSOR_DISABLED);
}

void FocusedWindow_s::setFocusedWindow(Window_s* ptr) { m_ptr = ptr; }

Window_s* FocusedWindow_s::operator()() const
{ //
    return m_ptr;
}

namespace detail
{
    WindowEventDispatcher::WindowEventDispatcher(Window_s* ptr) : m_ptr(ptr) {}

    void WindowEventDispatcher::onKey(
      I32_t key,
      I32_t scancode,
      I32_t action,
      I32_t mods) const
    { //
        m_ptr->onKey(key, scancode, action, mods);
    }

    void WindowEventDispatcher::onMouseButton(
      I32_t button,
      I32_t action,
      I32_t mods) const
    { //
        m_ptr->onMouseButton(button, action, mods);
    }

    void WindowEventDispatcher::onCursorMovement(F32_t xpos, F32_t ypos) const
    { //
        m_ptr->onCursorMovement(xpos, ypos);
    }

    void
      WindowEventDispatcher::onFramebufferSize(I32_t width, I32_t height) const
    { //
        m_ptr->onFramebufferSize(width, height);
    }
} // namespace detail

} // namespace cge
