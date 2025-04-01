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
  RHI::IContext * ctx = reinterpret_cast<RHI::IContext *>(glfwGetWindowUserPointer(window));
  ShouldInvalidateScene = true;
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

  RHI::ImageCreateArguments args;
  args.extent = {100, 100, 1};
  args.format = RHI::ImageFormat::RGBA8;
  args.mipLevels = 1; 
  args.samples = RHI::SamplesCount::One;
  args.shared = false;
  args.type = RHI::ImageType::Image2D;
  auto image =  
    ctx->AllocImage(args, static_cast<RHI::TextureUsage>(RHI::TextureUsage::Sampler |
                                                         RHI::TextureUsage::Sampler));


 // RHI::IFramebuffer * offscreen = ctx->CreateFramebuffer(3);
  //offscreen->AddImageAttachment(0, image);

  RHI::IFramebuffer * windowed = ctx->CreateFramebuffer(3);
  windowed->AddImageAttachment(0, ctx->GetSurfaceImage());

  auto windowedSubpass = windowed->CreateSubpass();
  {
    auto && subpassConfig = windowedSubpass->GetConfiguration();
    // set shaders
    subpassConfig.AttachShader(RHI::ShaderType::Vertex,
                               std::filesystem::path(SHADERS_FOLDER) / "quad.vert");
    subpassConfig.AttachShader(RHI::ShaderType::Fragment,
                               std::filesystem::path(SHADERS_FOLDER) / "quad.frag");
    subpassConfig.SetMeshTopology(RHI::MeshTopology::TriangleFan); 
    //auto * sampler = subpassConfig.DeclareSampler(0, RHI::ShaderType::Fragment);
    //sampler->AssignImage(image);
  }

  // create vertex buffer
  //auto && vertexBuffer =
  //  ctx->AllocBuffer(VerticesCount * 5 * sizeof(float), RHI::BufferGPUUsage::VertexBuffer);
  //vertexBuffer->UploadSync(Vertices, VerticesCount * 5 * sizeof(float));

  //// create index buffer
  //auto indexBuffer =
  //  ctx->AllocBuffer(IndicesCount * sizeof(uint32_t), RHI::BufferGPUUsage::IndexBuffer);
  //indexBuffer->UploadSync(Indices, IndicesCount * sizeof(uint32_t));

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    if (auto * renderTarget = windowed->BeginFrame())
    {
      renderTarget->SetClearValue(0, 0.1f, 0.7f, 0.4f, 1.0f);
      if (ShouldInvalidateScene || windowedSubpass->ShouldBeInvalidated())
      {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        windowedSubpass->BeginPass();
        windowedSubpass->SetViewport(static_cast<float>(width), static_cast<float>(height));
        windowedSubpass->SetScissor(0, 0, static_cast<uint32_t>(width),
                                    static_cast<uint32_t>(height));
        windowedSubpass->DrawVertices(4, 1);
        windowedSubpass->EndPass();
        ShouldInvalidateScene = false;
      }
      windowed->EndFrame();
    }
  }

  glfwTerminate();
  return 0;
}
