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
  ctx->GetSurfaceSwapchain()->Invalidate();
  ShouldInvalidateScene = true;
}

static constexpr uint32_t VerticesCount = 4;
static constexpr float Vertices[] = {
  // pos + colors(rgb)
  0.5f,  0.5f,  0.227f, 0.117f, 0.729f, /*first vertex*/
  -0.5f, 0.5f,  0.729f, 0.117f, 0.227f, /*second vertex*/
  -0.5f, -0.5f, 0.619f, 0.729f, 0.117f, /*third vertex*/
  0.5f,  -0.5f, 0.117f, 0.729f, 0.533f  /*fourth vertex*/
};

static constexpr uint32_t IndicesCount = 6;
static constexpr uint32_t Indices[] = {0, 1, 2, 0, 2, 3};

int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // Create GLFW window
  GLFWwindow * window = glfwCreateWindow(800, 600, "Uniforms_RHI", NULL, NULL);
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

  // create buffers for each uniform variables
  auto tBuf = ctx->AllocBuffer(sizeof(float), RHI::BufferGPUUsage::UniformBuffer);
  auto transformBuf = ctx->AllocBuffer(2 * sizeof(float), RHI::BufferGPUUsage::UniformBuffer);

  auto * swapchain = ctx->GetSurfaceSwapchain();
  auto * subpass = swapchain->CreateSubpass();
  auto && trianglePipeline = subpass->GetConfiguration();
  trianglePipeline.AttachShader(RHI::ShaderType::Vertex,
                                std::filesystem::path(SHADERS_FOLDER) / "uniform.vert");
  trianglePipeline.AttachShader(RHI::ShaderType::Fragment,
                                std::filesystem::path(SHADERS_FOLDER) / "uniform.frag");
  // set vertex attributes (5 float attributes per vertex - pos.xy and color.rgb)
  trianglePipeline.AddInputBinding(0, 5 * sizeof(float), RHI::InputBindingType::VertexData);
  trianglePipeline.AddInputAttribute(0, 0, 0, 2, RHI::InputAttributeElementType::FLOAT);
  trianglePipeline.AddInputAttribute(0, 1, 2 * sizeof(float), 3,
                                     RHI::InputAttributeElementType::FLOAT);

  // declare uniform variables
  auto && u_t =
    trianglePipeline.DeclareUniform(0, RHI::ShaderType::Fragment | RHI::ShaderType::Vertex);
  u_t->Invalidate();
  u_t->AssignBuffer(*tBuf); // bind buffer to uniform variable

  auto && u_transform = trianglePipeline.DeclareUniform(1, RHI::ShaderType::Vertex);
  u_transform->Invalidate();
  u_transform->AssignBuffer(*transformBuf); // bind buffer to uniform variable

  // create vertex buffer
  auto && vertexBuffer =
    ctx->AllocBuffer(VerticesCount * 5 * sizeof(float), RHI::BufferGPUUsage::VertexBuffer);
  vertexBuffer->UploadSync(Vertices, VerticesCount * 5 * sizeof(float));

  // create index buffer
  auto indexBuffer =
    ctx->AllocBuffer(IndicesCount * sizeof(uint32_t), RHI::BufferGPUUsage::IndexBuffer);
  indexBuffer->UploadSync(Indices, IndicesCount * sizeof(uint32_t));

  float x = 0.0f;
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    float t_val = std::abs(std::sin(x));
    tBuf->UploadSync(&t_val, sizeof(float));

    std::pair<float, float> transform_val{std::cos(x), std::sin(x)};
    transformBuf->UploadAsync(&transform_val, 2 * sizeof(float));

    x += 0.0001f;
    ctx->GetTransferer()->Flush();

    if (RHI::IRenderTarget * renderTarget = swapchain->AcquireFrame())
    {
      renderTarget->SetClearColor(0.3f, 0.3f, 0.5f, 1.0f);
      if (ShouldInvalidateScene || subpass->ShouldBeInvalidated())
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
  }

  glfwTerminate();
  return 0;
}
