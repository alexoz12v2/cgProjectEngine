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

class Window_s;
namespace detail
{
    class WindowEventDispatcher
    {
      public:
        WindowEventDispatcher(Window_s *);

      public:
        void onKey(I32_t key, I32_t scancode, I32_t action, I32_t mods) const;
        void onMouseButton(I32_t button, I32_t action, I32_t mods) const;
        void onCursorMovement(F32_t xpos, F32_t ypos) const;
        void onFramebufferSize(I32_t width, I32_t height) const;

      private:
        Window_s *m_ptr = nullptr;
    };
} // namespace detail

class Window_s
{
    friend detail::WindowEventDispatcher;

  public:
    Window_s()                                 = default;
    Window_s(Window_s const &)                 = default;
    Window_s(Window_s &&)                      = default;
    Window_s &operator=(Window_s const &other) = default;
    Window_s &operator=(Window_s &&other)      = default;
    ~Window_s();

  public:
    EErr_t init(WindowSpec_t const &spec);
    bool   shouldClose();
    void   swapBuffers();
    void   pollEvents(I32_t waitMillis = 0) const;

    void       emitFramebufferSize() const;
    glm::ivec2 getFramebufferSize() const;

    void *internal();

    void enableCursor();
    void disableCursor();

  private:
    void onKey(I32_t key, I32_t scancode, I32_t action, I32_t mods) const;
    void onMouseButton(I32_t button, I32_t action, I32_t mods) const;
    void onCursorMovement(F32_t xpos, F32_t ypos) const;
    void onFramebufferSize(I32_t width, I32_t height) const;

  private:
    gsl::owner<void *> m_handle = nullptr;
};

class FocusedWindow_s
{
  public:
    Window_s *operator()() const;
    void      setFocusedWindow(Window_s *ptr);

  private:
    Window_s *m_ptr = nullptr;
};

extern FocusedWindow_s g_focusedWindow;

} // namespace cge
