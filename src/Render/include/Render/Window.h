#pragma once

#include "Core/Type.h"

#include <gsl/pointers>

struct GLFWwindow;

namespace cge
{

struct WindowSpec_t
{
    Char8_t const *title;
    I32_t          width;
    I32_t          height;
};

class Window_s
{
  public:
    Window_s()                                 = default;
    Window_s(Window_s const &)                 = default;
    Window_s(Window_s &&)                      = default;
    Window_s &operator=(Window_s const &other) = default;
    Window_s &operator=(Window_s &&other)      = default;
    ~Window_s();

    EErr_t init(WindowSpec_t const &spec);
    bool   shouldClose();
    void   swapBuffers();
    void   pollEvents(I32_t waitMillis = 0) const;

    void setDeltaTime(F32_t dt) { deltaTime = dt; }
    void emitFramebufferSize() const;

  private:
    gsl::owner<GLFWwindow *> handle    = nullptr;
    F32_t                    deltaTime = 0.16f;

    static void keyCallback(
      GLFWwindow *window,
      I32_t       key,
      I32_t       scancode,
      I32_t       action,
      I32_t       mods);

    static void errorCallback(I32_t error, const char *description);
    static void
      cursorPositionCallback(GLFWwindow *window, F64_t xpos, F64_t ypos);
    static void mouseButtonCallback(
      GLFWwindow *window,
      I32_t       button,
      I32_t       action,
      I32_t       mods);

    static void
      framebufferSizeCallback(GLFWwindow *window, I32_t width, I32_t height);

    void onKey(I32_t key, I32_t scancode, I32_t action, I32_t mods) const;
    void onMouseButton(I32_t button, I32_t action, I32_t mods) const;
    void onCursorMovement(F32_t xpos, F32_t ypos) const;
    void onFramebufferSize(I32_t width, I32_t height) const;
};
} // namespace cge
