#include <cmath>
#include <cstdio>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include <RHI.hpp>

#include "stb_image.h"
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
  ctx->GetSurfaceSwapchain()->Invalidate();
  ShouldInvalidateScene = true;
}

/// @brief uploads image from file and create RHI image object
std::unique_ptr<RHI::IImageGPU> CreateAndLoadImage(const RHI::IContext & ctx, const char * path)
{
  int w = 0, h = 0, channels = 3;
  uint8_t * pixel_data = stbi_load(path, &w, &h, &channels, STBI_rgb);
  if (!pixel_data)
  {
    stbi_image_free(pixel_data);
    throw std::runtime_error("Failed to load texture. Check it exists near the exe file");
  }

  RHI::ImageCreateArguments imageArgs{};
  imageArgs.width = w;
  imageArgs.height = h;
  imageArgs.depth = 1;
  imageArgs.type = RHI::ImageType::Image2D;
  imageArgs.shared = false;
  imageArgs.format = channels == 4 ? RHI::ImageFormat::RGBA8 : RHI::ImageFormat::RGB8;
  imageArgs.mipLevels = 1;
  imageArgs.samples = RHI::SamplesCount::One;
  imageArgs.usage = RHI::ImageGPUUsage::Sample;
  auto texture = ctx.AllocImage(imageArgs);
  texture->UploadAsync(pixel_data, w * h * sizeof(int));
  stbi_image_free(pixel_data);
  return texture;
}

using mat3 = std::array<float, 9>;

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

  auto texture1 = CreateAndLoadImage(*ctx, "texture1.png");
  auto texture2 = CreateAndLoadImage(*ctx, "texture2.png");

  auto * swapchain = ctx->GetSurfaceSwapchain();
  auto * subpass = swapchain->CreateSubpass();
  // create pipeline for triangle. Here we can configure gpu pipeline for rendering
  auto && trianglePipeline = subpass->GetConfiguration();
  trianglePipeline.AttachShader(RHI::ShaderType::Vertex,
                                std::filesystem::path(SHADERS_FOLDER) / "textured_quad.vert");
  trianglePipeline.AttachShader(RHI::ShaderType::Fragment,
                                std::filesystem::path(SHADERS_FOLDER) / "textured_quad.frag");

  auto && texSampler = trianglePipeline.DeclareSampler("texSampler", 0, RHI::ShaderType::Fragment);
  auto && transform_buf =
    trianglePipeline.DeclareUniform("u_transform", 1, RHI::ShaderType::Vertex, sizeof(mat3));

  ShouldInvalidateScene = true;
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    ctx->GetTransferer()->Flush();

    if (RHI::IRenderTarget * renderTarget = swapchain->AcquireFrame())
    {
      renderTarget->SetClearColor(0.3f, 0.3f, 0.5f, 1.0f);
      if (ShouldInvalidateScene)
      {
        // get size of window
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        subpass->BeginPass();
        // set viewport
        subpass->SetViewport(static_cast<float>(width), static_cast<float>(height));
        // set scissor
        subpass->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        
        // draw first quad
        texSampler->GetImageView().AssignImage(*texture1);
        texSampler->Invalidate();
        subpass->DrawVertices(6, 1);

        //draw second quad
        texSampler->GetImageView().AssignImage(*texture1);
        texSampler->Invalidate();
        subpass->DrawVertices(6, 1);
        subpass->EndPass();

        ShouldInvalidateScene = false;
      }

      swapchain->FlushFrame();
    }
  }

  // wait while gpu is idle to destroy context and its objects correctly
  ctx->WaitForIdle();
  glfwTerminate();
  return 0;
}
