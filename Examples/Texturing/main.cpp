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
  }
}

// flag means that you should clear and update trianglePipelineCommands (see in main)
bool ShouldInvalidateScene = true;
bool ShouldSwitchNextImage = false;

// Resize window callback
void OnResizeWindow(GLFWwindow * window, int width, int height)
{
  RHI::IContext * ctx = reinterpret_cast<RHI::IContext *>(glfwGetWindowUserPointer(window));
  ShouldInvalidateScene = true;
}

void OnKeyPressed(GLFWwindow * window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
  {
    ShouldSwitchNextImage = true;
  }
}

struct PushConstant
{
  float scale_x;
  float scale_y;
  float pos_x;
  float pos_y;
  uint32_t texture_index;
};


/// @brief uploads image from file and create RHI image object
RHI::ITexture * CreateAndLoadImage(RHI::IContext & ctx, const char * path, bool with_alpha)
{
  int w = 0, h = 0, channels = 3;
  uint8_t * pixel_data = stbi_load(path, &w, &h, &channels, with_alpha ? STBI_rgb_alpha : STBI_rgb);
  if (!pixel_data)
  {
    throw std::runtime_error("Failed to load texture. Check it exists near the exe file");
  }

  RHI::ImageExtent extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};

  RHI::ImageCreateArguments imageArgs{};
  imageArgs.extent = extent;
  imageArgs.type = RHI::ImageType::Image2D;
  imageArgs.shared = false;
  imageArgs.format = with_alpha ? RHI::ImageFormat::RGBA8 : RHI::ImageFormat::RGB8;
  imageArgs.mipLevels = 1;
  imageArgs.samples = RHI::SamplesCount::One;
  auto texture = ctx.AllocImage(imageArgs);
  RHI::CopyImageArguments copyArgs{};
  copyArgs.hostFormat = with_alpha ? RHI::HostImageFormat::RGBA8 : RHI::HostImageFormat::RGB8;
  copyArgs.src.extent = extent;
  copyArgs.dst.extent = extent;
  texture->UploadImage(pixel_data, copyArgs);
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
  glfwSetKeyCallback(window, OnKeyPressed);

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

  std::vector<RHI::ITexture *> textures;
  textures.emplace_back(CreateAndLoadImage(*ctx, "texture.png", true));
  textures.emplace_back(CreateAndLoadImage(*ctx, "jackal.jpg", false));
  textures.emplace_back(CreateAndLoadImage(*ctx, "shrek1.jpg", false));
  textures.emplace_back(CreateAndLoadImage(*ctx, "shrek2.jpg", false));
  textures.emplace_back(CreateAndLoadImage(*ctx, "mike_wazowski.jpg", false));
  textures.emplace_back(CreateAndLoadImage(*ctx, "pepe.jpg", false));
  auto image_it = textures.begin();

  RHI::IFramebuffer * framebuffer = ctx->CreateFramebuffer(3);
  framebuffer->AddAttachment(0, ctx->GetSurfaceImage());
  auto * subpass = framebuffer->CreateSubpass();
  // create pipeline for triangle. Here we can configure gpu pipeline for rendering
  auto && trianglePipeline = subpass->GetConfiguration();
  trianglePipeline.BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
  trianglePipeline.AttachShader(RHI::ShaderType::Vertex, "textures.vert");
  trianglePipeline.AttachShader(RHI::ShaderType::Fragment, "textures.frag");
  trianglePipeline.DefinePushConstant(sizeof(PushConstant),
                                      RHI::ShaderType::Fragment | RHI::ShaderType::Vertex);

  std::vector<RHI::ISamplerUniformDescriptor *> samplers;
  auto it = textures.begin();
  for (uint32_t i = 0; i < 8; ++i)
  {
    auto * texture = trianglePipeline.DeclareSampler(i, RHI::ShaderType::Fragment);
    texture->AssignImage(*it);
    samplers.push_back(texture);
    it = std::next(it);
    if (it == textures.end())
      it = textures.begin();
  }

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    ctx->Flush();

    if (RHI::IRenderTarget * renderTarget = framebuffer->BeginFrame())
    {
      renderTarget->SetClearValue(0, 0.3f, 0.3f, 0.5f, 1.0f);

      // change textures at realtime
      if (ShouldSwitchNextImage)
      {
        image_it = std::next(image_it);
        if (image_it == textures.end())
          image_it = textures.begin();
        auto it = image_it;
        for (auto && sampler : samplers)
        {
          sampler->AssignImage(*it);
          it = std::next(it);
          if (it == textures.end())
            it = textures.begin();
        }
        ShouldSwitchNextImage = false;
      }

      if (ShouldInvalidateScene || subpass->ShouldBeInvalidated())
      {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        subpass->BeginPass();
        subpass->SetViewport(static_cast<float>(width), static_cast<float>(height));
        subpass->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        constexpr int cells_count_in_row = 8;
        constexpr float cell_width = 2.0f / cells_count_in_row;
        constexpr float offset = -1.0f + cell_width / 2.0f;
        constexpr float margin = 0.05f;
        PushConstant ct;
        ct.scale_x = cell_width / 2.0f;
        ct.scale_y = cell_width / 2.0f;
        ct.texture_index = 0;
        for (int i = 0; i <= cells_count_in_row; ++i)
        {
          for (int j = 0; j <= cells_count_in_row; ++j)
          {
            ct.pos_x = offset + j * (cell_width + margin);
            ct.pos_y = offset + i * (cell_width + margin);
            ct.texture_index = (ct.texture_index + 1) % textures.size();
            subpass->PushConstant(&ct, sizeof(PushConstant));
            subpass->DrawVertices(6, 1);
          }
        }

        subpass->EndPass();
        ShouldInvalidateScene = false;
      }
      framebuffer->EndFrame();
    }
  }

  glfwTerminate();
  return 0;
}
