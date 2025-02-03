#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <future>

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

static constexpr uint32_t VerticesCount = 3;
static constexpr float Vertices[] = {
  // pos + colors(rgb)
  0.5f,  0.5f,  1.0f, 0.0f, 0.0f, /*first vertex*/
  -0.5f, 0.5f,  0.0f, 1.0f, 0.0f, /*second vertex*/
  -0.0f, -0.5f, 0.0f, 0.0f, 1.0f  /*third vertex*/
};

static constexpr uint32_t IndicesCount = 3;
static constexpr uint32_t Indices[] = {0, 1, 2};

// helper which incapsulated rendering code for some scene
struct Renderer
{
  explicit Renderer(RHI::IContext & ctx, RHI::ISwapchain & swapchain, GLFWwindow * window);

  // draw scene in parallel
  void AsyncDrawScene()
  {
    if (m_drawTask.valid())
    {
      // wait for previous task
      auto result = m_drawTask.wait_for(std::chrono::milliseconds(10));
      if (result == std::future_status::timeout)
        return;
    }
    auto && future = std::async(&Renderer::DrawSceneImpl, this);
    m_drawTask = std::move(future);
  }

  void UpdateGeometry()
  {
    m_vertexBuffer->UploadAsync(Vertices, VerticesCount * 5 * sizeof(float));
    m_indexBuffer->UploadAsync(Indices, IndicesCount * sizeof(uint32_t));
    if (m_subpass->ShouldBeInvalidated())
      AsyncDrawScene();
  }

private:
  /// weak pointer on Window. Just for getting size of it.
  GLFWwindow * m_windowPtr;
  /// subpass which can be executed in parallel
  RHI::ISubpass * m_subpass;

  /// some data for frame
  std::unique_ptr<RHI::IBufferGPU> m_vertexBuffer;
  std::unique_ptr<RHI::IBufferGPU> m_indexBuffer;

  /// current draw task
  std::future<bool> m_drawTask;

private:
  bool DrawSceneImpl();
};
std::unique_ptr<Renderer> TriangleRenderer;

// Resize window callback
void OnResizeWindow(GLFWwindow * window, int width, int height)
{
  RHI::IContext * ctx = reinterpret_cast<RHI::IContext *>(glfwGetWindowUserPointer(window));
  TriangleRenderer->AsyncDrawScene();
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
  TriangleRenderer = std::make_unique<Renderer>(*ctx, *swapchain, window);
  TriangleRenderer->AsyncDrawScene();

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
    TriangleRenderer->UpdateGeometry();
    ctx->Flush();
    if (auto * renderTarget = swapchain->AcquireFrame())
    {
      renderTarget->SetClearValue(0, 0.1f, 1.0f, 0.4f, 1.0f);
      swapchain->RenderFrame();
    }
  }

  TriangleRenderer.reset();
  glfwTerminate();
  return 0;
}

Renderer::Renderer(RHI::IContext & ctx, RHI::ISwapchain & swapchain, GLFWwindow * window)
  : m_windowPtr(window)
{
  // create pipeline for triangle. Here we can configure gpu pipeline for rendering
  m_subpass = swapchain.CreateSubpass();
  auto && trianglePipeline = m_subpass->GetConfiguration();
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
  m_vertexBuffer =
    ctx.AllocBuffer(VerticesCount * 5 * sizeof(float), RHI::BufferGPUUsage::VertexBuffer);
  m_vertexBuffer->UploadAsync(Vertices, VerticesCount * 5 * sizeof(float));


  // create index buffer
  m_indexBuffer =
    ctx.AllocBuffer(IndicesCount * sizeof(uint32_t), RHI::BufferGPUUsage::IndexBuffer);
  m_indexBuffer->UploadAsync(Indices, IndicesCount * sizeof(uint32_t));
}

bool Renderer::DrawSceneImpl()
{
  int width, height;
  glfwGetFramebufferSize(m_windowPtr, &width, &height);
  m_subpass->BeginPass();
  m_subpass->SetViewport(static_cast<float>(width), static_cast<float>(height));
  m_subpass->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
  m_subpass->BindVertexBuffer(0, *m_vertexBuffer, 0);
  m_subpass->BindIndexBuffer(*m_indexBuffer, RHI::IndexType::UINT32);
  m_subpass->DrawIndexedVertices(IndicesCount, 1);
  m_subpass->EndPass();
  return true;
}
