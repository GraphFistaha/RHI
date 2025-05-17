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

// flag means that you should clear and update trianglePipelineCommands (see in main)
bool ShouldInvalidateScene = true;

// Resize window callback
void OnResizeWindow(GLFWwindow * window, int width, int height)
{
  ShouldInvalidateScene = true;
}

int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // Create GLFW window
  GLFWwindow * wnd1 = glfwCreateWindow(800, 600, "Window1_RHI", NULL, NULL);
  if (wnd1 == NULL)
  {
    std::printf("Failed to create GLFW window 1\n");
    glfwTerminate();
    return -1;
  }
  GLFWwindow * wnd2 = glfwCreateWindow(800, 600, "Window2_RHI", NULL, NULL);
  if (wnd2 == NULL)
  {
    std::printf("Failed to create GLFW window 2\n");
    glfwTerminate();
    return -1;
  }
  // set callback on resize
  glfwSetWindowSizeCallback(wnd1, OnResizeWindow);
  // set callback on resize
  glfwSetWindowSizeCallback(wnd2, OnResizeWindow);

  // fill structure for surface with OS handles
  RHI::SurfaceConfig surface1{};
  RHI::SurfaceConfig surface2{};
#ifdef _WIN32
  surface1.hWnd = glfwGetWin32Window(wnd1);
  surface1.hInstance = GetModuleHandle(nullptr);

  surface2.hWnd = glfwGetWin32Window(wnd2);
  surface2.hInstance = GetModuleHandle(nullptr);
#elif defined(__linux__)
  surface1.hWnd = reinterpret_cast<void *>(glfwGetX11Window(wnd1));
  surface1.hInstance = glfwGetX11Display();

  surface2.hWnd = reinterpret_cast<void *>(glfwGetX11Window(wnd2));
  surface2.hInstance = glfwGetX11Display();
#endif

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  RHI::IFramebuffer * fbo1 = ctx->CreateFramebuffer(3);
  auto subpass1 = fbo1->CreateSubpass();
  {
    fbo1->AddAttachment(0, ctx->CreateSurfacedAttachment(surface1));
    auto && pipeline = subpass1->GetConfiguration();
    pipeline.BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
    // set shaders
    pipeline.AttachShader(RHI::ShaderType::Vertex, "triangle.vert");
    pipeline.AttachShader(RHI::ShaderType::Fragment, "triangle_quad.frag");
    pipeline.SetMeshTopology(RHI::MeshTopology::Triangle);
  }

  RHI::IFramebuffer * fbo2 = ctx->CreateFramebuffer(3);
  auto subpass2 = fbo2->CreateSubpass();
  {
    fbo2->AddAttachment(0, ctx->CreateSurfacedAttachment(surface2));
    auto && pipeline = subpass2->GetConfiguration();
    pipeline.BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
    // set shaders
    pipeline.AttachShader(RHI::ShaderType::Vertex, "quad.vert");
    pipeline.AttachShader(RHI::ShaderType::Fragment, "triangle_quad.frag");
    pipeline.SetMeshTopology(RHI::MeshTopology::TriangleStrip);
  }

  float t = 0.0;
  while (!glfwWindowShouldClose(wnd1) && !glfwWindowShouldClose(wnd2))
  {
    glfwPollEvents();

    // render into wnd1
    if (auto * renderTarget = fbo1->BeginFrame())
    {
      renderTarget->SetClearValue(0, 0.1f, std::abs(std::sin(t)), 0.4f, 1.0f);

      if (ShouldInvalidateScene || subpass1->ShouldBeInvalidated())
      {
        int width, height;
        glfwGetFramebufferSize(wnd1, &width, &height);
        subpass1->BeginPass(); // begin drawing pass
        subpass1->SetViewport(static_cast<float>(width), static_cast<float>(height));
        subpass1->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        subpass1->DrawVertices(3, 1);
        subpass1->EndPass(); // finish drawing pass
      }
      fbo1->EndFrame();
    }

    // render into wnd1
    if (auto * renderTarget = fbo2->BeginFrame())
    {
      renderTarget->SetClearValue(0, 0.1f, std::abs(std::cos(t)), 0.4f, 1.0f);

      if (ShouldInvalidateScene || subpass2->ShouldBeInvalidated())
      {
        int width, height;
        glfwGetFramebufferSize(wnd2, &width, &height);
        subpass2->BeginPass(); // begin drawing pass
        subpass2->SetViewport(static_cast<float>(width), static_cast<float>(height));
        subpass2->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        subpass2->DrawVertices(4, 1);
        subpass2->EndPass(); // finish drawing pass
      }
      fbo2->EndFrame();
    }
    ShouldInvalidateScene = false;
    t += 0.001f;
  }

  glfwTerminate();
  return 0;
}
