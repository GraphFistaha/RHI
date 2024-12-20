#include <cstdio>
#include <cstring>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

struct VertexData
{
  float ndc_x;
  float ndc_y;
  float uv_x;
  float uv_y;
};

static constexpr uint32_t VerticesCount = 4;
static constexpr VertexData Vertices[] = {
  VertexData{0.5f, 0.5f, 0.0f, 0.0f},   /*first vertex*/
  VertexData{-0.5f, 0.5f, 1.0f, 0.0f},  /*second vertex*/
  VertexData{-0.5f, -0.5f, 1.0f, 1.0f}, /*third vertex*/
  VertexData{0.5f, -0.5f, 0.0f, 1.0f}   /*fourth vertex*/
};

static constexpr uint32_t IndicesCount = 6;
static constexpr uint32_t Indices[] = {0, 1, 2, 0, 2, 3};

/// @brief uploads image from file and create RHI image object
std::unique_ptr<RHI::IImageGPU> CreateAndLoadImage(const RHI::IContext & ctx, const char * path)
{
  int w = 0, h = 0, channels = 4;
  uint8_t * pixel_data = stbi_load(path, &w, &h, &channels, STBI_rgb_alpha);
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

int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // Create GLFW window
  GLFWwindow * window = glfwCreateWindow(800, 600, "Texturing_RHI", NULL, NULL);
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

  auto texture = CreateAndLoadImage(*ctx, "texture.png");

  auto * swapchain = ctx->GetSurfaceSwapchain();
  auto * subpass = swapchain->CreateSubpass();
  // create pipeline for triangle. Here we can configure gpu pipeline for rendering
  auto && trianglePipeline = subpass->GetConfiguration();
  trianglePipeline.AttachShader(RHI::ShaderType::Vertex,
                                std::filesystem::path(SHADERS_FOLDER) / "textures.vert");
  trianglePipeline.AttachShader(RHI::ShaderType::Fragment,
                                std::filesystem::path(SHADERS_FOLDER) / "textures.frag");
  trianglePipeline.AddInputBinding(0, sizeof(VertexData), RHI::InputBindingType::VertexData);
  trianglePipeline.AddInputAttribute(0, 0, offsetof(VertexData, ndc_x), 2,
                                     RHI::InputAttributeElementType::FLOAT);
  trianglePipeline.AddInputAttribute(0, 1, offsetof(VertexData, uv_x), 2,
                                     RHI::InputAttributeElementType::FLOAT);

  auto && tbuf =
    trianglePipeline.DeclareUniform("ub", 0, RHI::ShaderType::Fragment | RHI::ShaderType::Vertex,
                                    sizeof(float));

  auto && texSampler = trianglePipeline.DeclareSampler("texSampler", 1, RHI::ShaderType::Fragment);
  texSampler->GetImageView().AssignImage(*texture);
  texSampler->Invalidate();

  // create vertex buffer
  auto && vertexBuffer =
    ctx->AllocBuffer(VerticesCount * sizeof(VertexData), RHI::BufferGPUUsage::VertexBuffer);
  vertexBuffer->UploadAsync(Vertices, VerticesCount * sizeof(VertexData));

  // create index buffer
  auto indexBuffer =
    ctx->AllocBuffer(IndicesCount * sizeof(uint32_t), RHI::BufferGPUUsage::IndexBuffer);
  indexBuffer->UploadAsync(Indices, IndicesCount * sizeof(uint32_t));

  ShouldInvalidateScene = true;
  float x = 0.0f;
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
        // draw triangle
        subpass->BindVertexBuffer(0, *vertexBuffer, 0);
        subpass->BindIndexBuffer(*indexBuffer, RHI::IndexType::UINT32);
        subpass->DrawIndexedVertices(IndicesCount, 1);
        subpass->EndPass();

        ShouldInvalidateScene = false;
      }

      swapchain->FlushFrame();
    }

    if (auto map = tbuf->Map())
    {
      float t_val = std::abs(std::sin(x));
      std::memcpy(map.get(), &t_val, sizeof(float));
    }
    x += 0.0001f;
  }

  // wait while gpu is idle to destroy context and its objects correctly
  ctx->WaitForIdle();
  glfwTerminate();
  return 0;
}
