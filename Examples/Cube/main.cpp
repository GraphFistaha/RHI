#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <GLFW/glfw3.h>
#include <RHI.hpp>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>
#include <glm/ext.hpp>
#include <glm/glm.hpp>

/// global constants
constexpr float g_SceneExtent = 100; ///< quad radius of scene
constexpr glm::uvec2 g_defaultWindowSize{800, 600};
constexpr double g_mouseSensetive = 0.01;
constexpr float g_FOV = glm::radians(45.0f);
constexpr float g_cameraSpeed = 0.05f; // adjust accordingly
constexpr glm::vec3 g_cameraUp(0.0f, 1.0f, 0.0f);


class CubesRenderer final
{
  static constexpr size_t g_MaxCubesCount = 20; ///< max cubes count in scene

  /// global data for all cubes
  struct UniformBlock
  {
    glm::mat4 vp = glm::identity<glm::mat4>(); ///< camera transform: view + projection
  };

public:
  /// describes one cube in scene
  struct CubeDescription
  {
    glm::vec3 pos{};
    glm::vec3 axis{0, 0, 1};
    float scale = 1.0f;
  };

  explicit CubesRenderer(RHI::IContext & ctx);
  ~CubesRenderer();

public: // Client API
  void BindDrawSurface(RHI::IFramebuffer * framebuffer);
  size_t AddCubeToScene(const CubeDescription & description);
  void DeleteCubeFromScene(size_t cubeId);
  void SetCameraTransform(const glm::mat4 & vp);
  void InvalidateScene();

public: // RenderThread API
  void Draw();

private:
  std::array<CubeDescription, g_MaxCubesCount> m_cubes;
  size_t m_cubesCount = 0; ///< count on scene

  RHI::IContext & m_context;
  RHI::IFramebuffer * m_drawSurface = nullptr; ///< surface where to draw

  RHI::IBufferGPU * m_uniformBuffer = nullptr; ///< buffer for uniform
  RHI::IBufferGPU * m_cubesBuffer = nullptr;
  RHI::ISubpass * m_renderPass = nullptr; ///< renderPass for rendering cubes

  /// current draw task
  std::future<bool> m_drawTask;
  bool m_invalidScene = true;
  bool m_invalidGeometry = false;

private:
  void DestroyHandles();
};

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


/// global state
RHI::IFramebuffer * g_defaultFramebuffer = nullptr;
std::unique_ptr<CubesRenderer> g_renderer = nullptr;
glm::mat4 g_projectionMatrix;
glm::dvec2 g_cursorPos{0.0};
glm::vec3 g_cameraPos{0};
glm::vec3 g_cameraDirection{0, 0, -1};
static bool g_pressedKeys[1024];

void OnCameraTransformUpdated()
{
  auto viewMatrix = glm::lookAt(g_cameraPos, g_cameraDirection + g_cameraPos, g_cameraUp);
  auto vp = g_projectionMatrix * viewMatrix;
  g_renderer->SetCameraTransform(vp);
}

glm::vec3 OrthogonalVector(const glm::vec3 & v1, const glm::vec3 & v2)
{
  return glm::normalize(glm::cross(v1, v2));
}

void ProcessCameraMovement()
{
  auto newCameraPos = g_cameraPos;
  constexpr std::array<int, 4> movement_keys{GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D};
  bool has_action = std::any_of(movement_keys.begin(), movement_keys.end(),
                                [](int key) { return g_pressedKeys[key]; });

  if (has_action)
  {
    const auto cameraFront = g_cameraDirection;
    const auto cameraRight = OrthogonalVector(cameraFront, g_cameraUp);

    if (g_pressedKeys[GLFW_KEY_W])
      newCameraPos += cameraFront * g_cameraSpeed;
    else if (g_pressedKeys[GLFW_KEY_A])
      newCameraPos -= cameraRight * g_cameraSpeed;
    else if (g_pressedKeys[GLFW_KEY_S])
      newCameraPos -= cameraFront * g_cameraSpeed;
    else if (g_pressedKeys[GLFW_KEY_D])
      newCameraPos += cameraRight * g_cameraSpeed;
    g_cameraPos = newCameraPos;
    OnCameraTransformUpdated();
  }
}

// Resize window callback
void OnResizeWindow(GLFWwindow * window, int width, int height)
{
  g_defaultFramebuffer->Resize(width, height);
  g_projectionMatrix = glm::perspectiveZO(g_FOV, static_cast<float>(width) / height, 0.1f, 100.0f);
  OnCameraTransformUpdated();
  g_renderer->InvalidateScene();
}

void OnCursorMoved(GLFWwindow * window, double xpos, double ypos)
{
  static bool st_firstCursorMovement = true;
  if (st_firstCursorMovement)
  {
    g_cursorPos = {xpos, ypos};
    st_firstCursorMovement = false;
    return;
  }

  glm::fvec2 offset{(xpos - g_cursorPos.x) * g_mouseSensetive,
                    // reversed since y-coordinates range from bottom to top
                    (g_cursorPos.y - ypos) * g_mouseSensetive};

  g_cursorPos = {xpos, ypos};

  glm::mat4 rotYaw = glm::rotate(glm::identity<glm::mat4>(), -offset.x, glm::vec3(0, 1, 0));
  glm::mat4 rotPitch = glm::identity<glm::mat4>();
  if (glm::abs(glm::dot(g_cameraDirection, g_cameraUp) - glm::radians(89.0f)) > 0.01f)
    rotPitch = glm::rotate(glm::identity<glm::mat4>(), -offset.y, glm::vec3(1, 0, 0));

  g_cameraDirection = glm::normalize(rotYaw * rotPitch * glm::vec4(g_cameraDirection, 0.0));
  OnCameraTransformUpdated();
}

void OnKeyAction(GLFWwindow * window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    return;
  }

  if (action == GLFW_PRESS)
    g_pressedKeys[key] = true;
  else if (action == GLFW_RELEASE)
    g_pressedKeys[key] = false;
}

int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // Create GLFW window
  GLFWwindow * window =
    glfwCreateWindow(g_defaultWindowSize.x, g_defaultWindowSize.y, "Cube_RHI", NULL, NULL);
  if (window == NULL)
  {
    std::printf("Failed to create GLFW window\n");
    glfwTerminate();
    return -1;
  }
  // set callback on resize
  glfwSetWindowSizeCallback(window, OnResizeWindow);
  glfwSetCursorPosCallback(window, OnCursorMoved);
  glfwSetKeyCallback(window, OnKeyAction);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  g_projectionMatrix =
    glm::perspectiveZO(g_FOV, static_cast<float>(g_defaultWindowSize.x) / g_defaultWindowSize.y,
                       0.1f, 100.0f);

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

  RHI::ImageCreateArguments depthStencilAttachmentDescription{};
  {
    depthStencilAttachmentDescription.format = RHI::ImageFormat::DEPTH_STENCIL;
    depthStencilAttachmentDescription.extent = {g_defaultWindowSize.x, g_defaultWindowSize.y, 1};
    depthStencilAttachmentDescription.mipLevels = 1;
    depthStencilAttachmentDescription.samples = RHI::SamplesCount::One;
    depthStencilAttachmentDescription.shared = false;
    depthStencilAttachmentDescription.type = RHI::ImageType::Image2D;
  }

  RHI::IFramebuffer * framebuffer = g_defaultFramebuffer = ctx->CreateFramebuffer(3);
  framebuffer->AddAttachment(0, ctx->GetSurfaceImage());
  framebuffer->AddAttachment(1, ctx->AllocAttachment(depthStencilAttachmentDescription));

  g_renderer = std::make_unique<CubesRenderer>(*ctx);
  g_renderer->BindDrawSurface(framebuffer);
  CubesRenderer::CubeDescription cube;
  cube.pos = {0, 0, -10};
  g_renderer->AddCubeToScene(cube);

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
    ProcessCameraMovement();
    ctx->Flush();

    if (auto * renderTarget = framebuffer->BeginFrame())
    {
      renderTarget->SetClearValue(0, 0.1f, 0.7f, 0.4f, 1.0f);
      renderTarget->SetClearValue(1, 1.0f, 0);
      framebuffer->EndFrame();
    }
    g_renderer->Draw(); //TODO: call it in separate thread

    ctx->ClearResources();
  }

  glfwTerminate();
  return 0;
}

//------------------------ Cubes Renderer implementation --------------------

CubesRenderer::CubesRenderer(RHI::IContext & ctx)
  : m_context(ctx)
  , m_uniformBuffer(ctx.AllocBuffer(sizeof(UniformBlock), RHI::BufferGPUUsage::UniformBuffer, true))
  , m_cubesBuffer(ctx.AllocBuffer(sizeof(CubeDescription) * g_MaxCubesCount,
                                  RHI::BufferGPUUsage::VertexBuffer, false))
{
  SetCameraTransform(glm::identity<glm::mat4>());
}

CubesRenderer::~CubesRenderer()
{
  //TODO: Destroy uniform buffer
  //TODO: destroy cubesBuffer
  DestroyHandles();
}

void CubesRenderer::BindDrawSurface(RHI::IFramebuffer * framebuffer)
{
  auto newSubpass = framebuffer->CreateSubpass();
  {
    auto && subpassConfig = newSubpass->GetConfiguration();
    subpassConfig.BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
    subpassConfig.BindAttachment(1, RHI::ShaderAttachmentSlot::DepthStencil);
    subpassConfig.EnableDepthTest(true);
    // set shaders
    subpassConfig.AttachShader(RHI::ShaderType::Vertex, "cube.vert");
    subpassConfig.AttachShader(RHI::ShaderType::Geometry, "cube.geom");
    subpassConfig.AttachShader(RHI::ShaderType::Fragment, "cube.frag");
    subpassConfig.SetMeshTopology(RHI::MeshTopology::Point);

    subpassConfig.AddInputBinding(0, sizeof(CubeDescription), RHI::InputBindingType::VertexData);
    subpassConfig.AddInputAttribute(0, 0, 0, 3, RHI::InputAttributeElementType::FLOAT);
    subpassConfig.AddInputAttribute(0, 1, 3 * sizeof(float), 3,
                                    RHI::InputAttributeElementType::FLOAT);
    subpassConfig.AddInputAttribute(0, 2, 6 * sizeof(float), 1,
                                    RHI::InputAttributeElementType::FLOAT);
    /*subpassConfig.AddInputAttribute(0, 3, 7 * sizeof(float), 1,
                                        RHI::InputAttributeElementType::UINT);*/

    auto * uniform = subpassConfig.DeclareUniform(0, RHI::ShaderType::Geometry);
    uniform->AssignBuffer(*m_uniformBuffer);
  }
  DestroyHandles();
  m_renderPass = newSubpass;
  m_drawSurface = framebuffer;
}

size_t CubesRenderer::AddCubeToScene(const CubeDescription & description)
{
  size_t result = m_cubesCount;
  m_cubes[m_cubesCount++] = description;
  m_invalidGeometry = true;
  return result;
}

void CubesRenderer::DeleteCubeFromScene(size_t cubeId)
{
  if (cubeId != m_cubesCount - 1)
    std::swap(m_cubes[cubeId], m_cubes[m_cubesCount - 1]);
  m_cubesCount--;
  m_invalidGeometry = true;
}

void CubesRenderer::SetCameraTransform(const glm::mat4 & vp)
{
  m_uniformBuffer->UploadAsync(&vp, sizeof(glm::mat4), offsetof(UniformBlock, vp));
}

void CubesRenderer::Draw()
{
  if (m_invalidGeometry && m_cubesCount > 0)
  {
    m_cubesBuffer->UploadAsync(m_cubes.data(), m_cubesCount * sizeof(CubeDescription));
    m_invalidGeometry = false;
  }

  if (m_invalidScene || m_renderPass->ShouldBeInvalidated())
  {
    auto extent = m_drawSurface->GetExtent();
    uint32_t width = extent[0], height = extent[1];
    m_renderPass->BeginPass();
    m_renderPass->SetViewport(static_cast<float>(width), static_cast<float>(height));
    m_renderPass->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    m_renderPass->BindVertexBuffer(0, *m_cubesBuffer);
    m_renderPass->DrawVertices(m_cubesCount, 1);

    m_renderPass->EndPass();
    m_invalidScene = false;
  }
}

void CubesRenderer::InvalidateScene()
{
  m_invalidScene = true;
}

void CubesRenderer::DestroyHandles()
{
  //TODO: Destroy buffers, renderPass
}
