#include "Window.hpp"

#include <cstdio>
#include <stdexcept>

#include <GLFW/glfw3.h>

#include "Window.hpp"
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>

#include "Window.hpp"

namespace
{
// Resize window callback
void OnResizeWindow(GLFWwindow * window, int width, int height)
{
  using namespace RHI::test_examples;
  Window * wnd = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
  if (wnd && wnd->onResize)
    wnd->onResize(width, height);
}

bool g_pressedKeys[GLFW_KEY_LAST]{false};

void OnKeyAction(GLFWwindow * window, int key, int scancode, int action, int mods)
{
  if (action == GLFW_PRESS)
    g_pressedKeys[key] = true;
  else if (action == GLFW_RELEASE)
    g_pressedKeys[key] = false;
}

void OnCursorMoved(GLFWwindow * window, double xpos, double ypos)
{
  using namespace RHI::test_examples;
  Window * wnd = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
  if (wnd && wnd->onResize)
    wnd->onMoveCursor(xpos, ypos);
}

} // namespace

namespace RHI::test_examples
{
#define TO_GLFW_WINDOW(x) reinterpret_cast<GLFWwindow *>((x))

Window::Window(const char * title, int width, int height)
{
  // Create GLFW window
  GLFWwindow * window = glfwCreateWindow(800, 600, "HelloTriangle_RHI", NULL, NULL);
  if (window == NULL)
  {
    throw std::runtime_error("Failed to create GLFW window");
  }
  // set callback on resize
  glfwSetWindowUserPointer(window, this);
  glfwSetWindowSizeCallback(window, OnResizeWindow);
  glfwSetCursorPosCallback(window, OnCursorMoved);
  glfwSetKeyCallback(window, OnKeyAction);
  impl = window;
}

Window::~Window()
{
  //glfwDestroyWindow(TO_GLFW_WINDOW(impl));
}

RHI::SurfaceConfig Window::GetDrawSurface() const noexcept
{
  // fill structure for surface with OS handles
  RHI::SurfaceConfig surface{};
#ifdef _WIN32
  surface.hWnd = glfwGetWin32Window(TO_GLFW_WINDOW(impl));
  surface.hInstance = GetModuleHandle(nullptr);
#elif defined(__linux__)
  surface.hWnd = reinterpret_cast<void *>(glfwGetX11Window(TO_GLFW_WINDOW(impl)));
  surface.hInstance = glfwGetX11Display();
#endif
  return surface;
}

std::pair<int, int> Window::GetSize() const noexcept
{
  int width, height;
  glfwGetFramebufferSize(TO_GLFW_WINDOW(impl), &width, &height);
  return {width, height};
}

void Window::MainLoop(TickCallback && tick)
{
  while (!glfwWindowShouldClose(TO_GLFW_WINDOW(impl)))
  {
    glfwPollEvents();
    if (tick)
      tick(0.0);
  }
}

void Window::Close()
{
  glfwSetWindowShouldClose(TO_GLFW_WINDOW(impl), GLFW_TRUE);
}

void Window::SetCursorHidden(bool hidden)
{
  glfwSetInputMode(TO_GLFW_WINDOW(impl), GLFW_CURSOR,
                   hidden ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

bool Window::IsCursorHidden() const noexcept
{
  return glfwGetInputMode(TO_GLFW_WINDOW(impl), GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
}

bool Window::IsKeyPressed(Keycode keycode) const
{
  return g_pressedKeys[keycode];
}

} // namespace RHI::test_examples
