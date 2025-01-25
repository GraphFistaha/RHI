#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <GLFW/glfw3.h>
#include <RHI.hpp>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>

// Custom log function used by RHI::Context
void ConsoleLog(RHI::LogMessageStatus status, const std::string & message)
{
  switch (status)
  {
    case RHI::LogMessageStatus::LOG_INFO:
      std::printf("INFO: - %s\n", message.c_str());
      break;
    case RHI::LogMessageStatus::LOG_WARNING:
      std::printf("WARNING: - %s\n", message.c_str());
      break;
    case RHI::LogMessageStatus::LOG_ERROR:
      std::printf("ERROR: - %s\n", message.c_str());
      break;
    case RHI::LogMessageStatus::LOG_DEBUG:
      std::printf("DEBUG: - %s\n", message.c_str());
      break;
  }
}

// Resize window callback
void OnResizeWindow(GLFWwindow * window, int width, int height)
{
  RHI::IContext * ctx = reinterpret_cast<RHI::IContext *>(glfwGetWindowUserPointer(window));
}


int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // Create GLFW window
  GLFWwindow * window = glfwCreateWindow(800, 600, "HelloTriangle_RHI", NULL, NULL);
  if (window == NULL)
  {
    std::printf("Failed to create GLFW window\n");
    glfwTerminate();
    return -1;
  }
  // set callback on resize
  glfwSetWindowSizeCallback(window, OnResizeWindow);

  // fill structure for surface with OS handles
  RHI::SurfaceConfig surface{};
#ifdef _WIN32
  surface.hWnd = glfwGetWin32Window(window);
  surface.hInstance = GetModuleHandle(nullptr);
#elif defined(__linux__)
  surface.hWnd = reinterpret_cast<void *>(glfwGetX11Window(window));
  surface.hInstance = glfwGetX11Display();
#endif

  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(&surface, ConsoleLog);
  glfwSetWindowUserPointer(window, ctx.get());

  RHI::ISwapchain * swapchain = ctx->GetSurfaceSwapchain();

  float t = 0.0;
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    if (RHI::IRenderTarget * renderTarget = swapchain->AcquireFrame())
    {
      renderTarget->SetClearValue(0, 0.1f, std::abs(std::sin(t)), 0.4f, 1.0f);
      swapchain->FlushFrame();
    }
    t += 0.001f;
  }

  glfwTerminate();
  return 0;
}
