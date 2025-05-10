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
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Renderer.hpp"

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

/// global constants
constexpr glm::uvec2 g_defaultWindowSize{800, 600};
constexpr double g_mouseSensetive = 0.01;
constexpr float g_cameraFOV = glm::radians(45.0f);
constexpr float g_cameraNearestPlane = 0.01f;
constexpr float g_cameraViewDistance = 100.0f;
constexpr float g_cameraSpeed = 0.1f; // adjust accordingly
constexpr glm::vec3 g_cameraUp(0.0f, -1.0f, 0.0f);
/// global state
RHI::IFramebuffer * g_defaultFramebuffer = nullptr;
std::unique_ptr<CubesRenderer> g_renderer = nullptr;
glm::mat4 g_projectionMatrix;
glm::vec3 g_cameraPos{0};
glm::vec3 g_cameraDirection{0, 0, -1};
bool g_pressedKeys[1024];
bool g_cursorHidden = true;

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
    else if (g_pressedKeys[GLFW_KEY_S])
      newCameraPos -= cameraFront * g_cameraSpeed;

    if (g_pressedKeys[GLFW_KEY_A])
      newCameraPos -= cameraRight * g_cameraSpeed;
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
  g_projectionMatrix = glm::perspectiveZO(g_cameraFOV, static_cast<float>(width) / height,
                                          g_cameraNearestPlane, g_cameraViewDistance);
  OnCameraTransformUpdated();
  g_renderer->InvalidateScene();
}

void OnCursorMoved(GLFWwindow * window, double xpos, double ypos)
{
  static bool st_firstCursorMovement = true;
  static glm::dvec2 g_cursorPos{0.0};
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

  glm::vec4 newCamDirection = glm::vec4(g_cameraDirection, 0.0);
  glm::mat4 rotYaw = glm::rotate(glm::identity<glm::mat4>(), offset.x, glm::vec3(0, 1, 0));
  newCamDirection = rotYaw * newCamDirection;
  if (glm::abs(glm::dot(g_cameraDirection, g_cameraUp) - glm::radians(89.0f)) > 0.01f)
  {
    auto right = OrthogonalVector(newCamDirection, g_cameraUp);
    glm::mat4 rotPitch = glm::rotate(glm::identity<glm::mat4>(), offset.y, -right);
    newCamDirection = rotPitch * newCamDirection;
  }
  g_cameraDirection = glm::normalize(newCamDirection);
  OnCameraTransformUpdated();
}

void OnKeyAction(GLFWwindow * window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    return;
  }

  if (key == GLFW_KEY_E && action == GLFW_PRESS)
  {
    g_cursorHidden = !g_cursorHidden;
    glfwSetInputMode(window, GLFW_CURSOR,
                     g_cursorHidden ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
  }

  if (action == GLFW_PRESS)
    g_pressedKeys[key] = true;
  else if (action == GLFW_RELEASE)
    g_pressedKeys[key] = false;
}

RHI::ITexture * UploadAndCreateTexture(const char * path, RHI::IContext & ctx, bool with_alpha)
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
  g_cursorHidden = true;
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  g_projectionMatrix =
    glm::perspectiveZO(g_cameraFOV,
                       static_cast<float>(g_defaultWindowSize.x) / g_defaultWindowSize.y,
                       g_cameraNearestPlane, g_cameraViewDistance);

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
  g_renderer->BindTexture(0, UploadAndCreateTexture("floor.jpg", *ctx, false));
  g_renderer->BindTexture(1, UploadAndCreateTexture("container.png", *ctx, false));
  {
    CubesRenderer::CubeDescription cube;
    cube.pos = {0, 0, 0};
    cube.scale = {10.0, 0.1, 10.0};
    cube.textureIndex = 0;
    g_renderer->AddCubeToScene(cube);
  }
  for (size_t i = 1; i < CubesRenderer::g_MaxCubesCount; ++i)
  {
    CubesRenderer::CubeDescription cube;
    cube.pos = glm::ballRand(10.0);
    cube.pos.y = glm::abs(cube.pos.y);
    cube.scale = glm::vec3(glm::gaussRand(0.5f, 0.49f));
    cube.textureIndex = 1;
    g_renderer->AddCubeToScene(cube);
  }

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
    ProcessCameraMovement();
    ctx->Flush();

    if (auto * renderTarget = framebuffer->BeginFrame())
    {
      renderTarget->SetClearValue(0, 0.1f, 0.2f, 0.4f, 1.0f);
      renderTarget->SetClearValue(1, 1.0f, 0);
      framebuffer->EndFrame();
    }
    g_renderer->Draw(); //TODO: call it in separate thread

    ctx->ClearResources();
  }

  glfwTerminate();
  return 0;
}
