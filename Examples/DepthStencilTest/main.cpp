#include <cmath>
#include <cstdio>
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

// flag means that you should clear and update trianglePipelineCommands (see in main)
bool ShouldInvalidateScene = true;

// Resize window callback
void OnResizeWindow(GLFWwindow * window, int width, int height)
{
  RHI::IContext * ctx = reinterpret_cast<RHI::IContext *>(glfwGetWindowUserPointer(window));
  ctx->GetSurfaceSwapchain()->SetInvalid();
  ShouldInvalidateScene = true;
}

using mat4 = std::array<float, 16>;
struct PushConstant
{
  mat4 transform;
  std::array<float, 4> color;
};

int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // Create GLFW window
  GLFWwindow * window = glfwCreateWindow(800, 600, "DepthStencilTest", NULL, NULL);
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

  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(surface, ConsoleLog);
  glfwSetWindowUserPointer(window, ctx.get());

  RHI::ImageCreateArguments args;

  auto * swapchain = ctx->GetSurfaceSwapchain();
  swapchain->AddImageAttachment(1, args);

  auto * subpass = swapchain->CreateSubpass();
  // create pipeline for triangle. Here we can configure gpu pipeline for rendering
  auto && trianglePipeline = subpass->GetConfiguration();
  trianglePipeline.AttachShader(RHI::ShaderType::Vertex,
                                std::filesystem::path(SHADERS_FOLDER) / "colored_quad.vert");
  trianglePipeline.AttachShader(RHI::ShaderType::Fragment,
                                std::filesystem::path(SHADERS_FOLDER) / "colored_quad.frag");
  trianglePipeline.DefinePushConstant(sizeof(PushConstant),
                                      RHI::ShaderType::Vertex | RHI::ShaderType::Fragment);
  //trianglePipeline.SetAttachmentUsage(RHI::ShaderImageSlot::Color, 0);
  //trianglePipeline.SetAttachmentUsage(RHI::ShaderImageSlot::DepthStencil, 1);

  ShouldInvalidateScene = true;
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    ctx->GetTransferer()->Flush();

    if (RHI::IRenderTarget * renderTarget = swapchain->AcquireFrame())
    {
      renderTarget->SetClearColor(0.3f, 0.3f, 0.5f, 1.0f);
      if (ShouldInvalidateScene || subpass->ShouldBeInvalidated())
      {
        PushConstant constant;
        constant.color = {1.0, 0.0, 0.0, 1.0};
        constant.transform = {0.25, 0,    0.0, 0.25, //
                              0.0,  0.25, 0.0, 0.25, //
                              0.0,  0.0,  1.0, 0.25, //
                              0.0,  0.0,  0.0, 1.0};
        // get size of window
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        subpass->BeginPass();
        // set viewport
        subpass->SetViewport(static_cast<float>(width), static_cast<float>(height));
        // set scissor
        subpass->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

        // draw first quad
        subpass->PushConstant(&constant, sizeof(constant));
        subpass->DrawVertices(6, 1);

        //draw second quad
        constant.color = {0.0, 1.0, 0.0, 1.0};
        constant.transform = {0.25, 0,    0.0, 0.0, //
                              0.0,  0.25, 0.0, 0.0, //
                              0.0,  0.0,  1.0, 0.0, //
                              0.0,  0.0,  0.0, 1.0};
        subpass->PushConstant(&constant, sizeof(constant));
        subpass->DrawVertices(6, 1);
        subpass->EndPass();

        ShouldInvalidateScene = false;
      }

      swapchain->FlushFrame();
    }
  }


  glfwTerminate();
  return 0;
}
