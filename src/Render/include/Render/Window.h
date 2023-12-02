#pragma once

#include "Core/Type.h"

#include <gsl/pointers>

struct GLFWwindow;

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
    void   pollEvents(I32_t waitMillis = 0);

  private:
    gsl::owner<GLFWwindow *> handle = nullptr;

    static void keyCallback(
      GLFWwindow *window,
      int         key,
      int         scancode,
      int         action,
      int         mods);

    static void errorCallback(int error, const char *description);

    static void
      framebufferSizeCallback(GLFWwindow *window, int width, int height);

    void onKey(int key, int scancode, int action, int mods);

    void onFramebufferSize(int width, int height);
};
