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

static constexpr uint32_t VerticesCount = 3;
static constexpr float Vertices[] = {
  // pos + colors(rgb)
  0.5f,  0.5f,  1.0f, 0.0f, 0.0f, /*first vertex*/
  -0.5f, 0.5f,  0.0f, 1.0f, 0.0f, /*second vertex*/
  -0.0f, -0.5f, 0.0f, 0.0f, 1.0f  /*third vertex*/
};

static constexpr uint32_t IndicesCount = 3;
static constexpr uint32_t Indices[] = {0, 1, 2};

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

  RHI::IRenderPass * swapchain = ctx->GetSurfaceSwapchain();

  // create pipeline for triangle. Here we can configure gpu pipeline for rendering
  auto subpass = swapchain->CreateSubpass();
  auto && trianglePipeline = subpass->GetConfiguration();
  // set shaders
  trianglePipeline.AttachShader(RHI::ShaderType::Vertex,
                                std::filesystem::path(SHADERS_FOLDER) / "triangle.vert");
  trianglePipeline.AttachShader(RHI::ShaderType::Fragment,
                                std::filesystem::path(SHADERS_FOLDER) / "triangle.frag");
  // set vertex attributes (5 float attributes per vertex - pos.xy and color.rgb)
  trianglePipeline.AddInputBinding(0, 5 * sizeof(float), RHI::InputBindingType::VertexData);
  trianglePipeline.AddInputAttribute(0, 0, 0, 2, RHI::InputAttributeElementType::FLOAT);
  trianglePipeline.AddInputAttribute(0, 1, 2 * sizeof(float), 3,
                                     RHI::InputAttributeElementType::FLOAT);

  // create vertex buffer
  auto && vertexBuffer =
    ctx->AllocBuffer(VerticesCount * 5 * sizeof(float), RHI::BufferGPUUsage::VertexBuffer);
  vertexBuffer->UploadSync(Vertices, VerticesCount * 5 * sizeof(float));

  // create index buffer
  auto indexBuffer =
    ctx->AllocBuffer(IndicesCount * sizeof(uint32_t), RHI::BufferGPUUsage::IndexBuffer);
  // fill buffer with Mapping into CPU memory
  if (auto scoped_map = indexBuffer->Map())
  {
    std::memcpy(scoped_map.get(), Indices, IndicesCount * sizeof(uint32_t));
    // in the end of scope mapping will be destroyed
  }
  // to make sure that buffer is sent on GPU
  indexBuffer->Flush();

  float t = 0.0;
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    if (auto * renderTarget = swapchain->BeginFrame())
    {
      renderTarget->SetClearValue(0, 0.1f, std::abs(std::sin(t)), 0.4f, 1.0f);

      // draw scene
      // ShouldInvalidateScene - assign true if you want refresh scene from client code
      // ShouldBeInvalidated() - returns true if scene should be redrawn because of internal changes
      if (ShouldInvalidateScene || subpass->ShouldBeInvalidated())
      {
        // get size of window
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        subpass->BeginPass(); // begin drawing pass
        // set viewport
        subpass->SetViewport(static_cast<float>(width), static_cast<float>(height));
        // set scissor
        subpass->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        // draw triangle
        subpass->BindVertexBuffer(0, *vertexBuffer, 0);
        subpass->BindIndexBuffer(*indexBuffer, RHI::IndexType::UINT32);
        subpass->DrawIndexedVertices(IndicesCount, 1);
        subpass->EndPass(); // finish drawing pass
        ShouldInvalidateScene = false;
      }
      swapchain->EndFrame();
    }
    t += 0.001f;
  }

  glfwTerminate();
  return 0;
}
