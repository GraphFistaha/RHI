#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <Camera.hpp>
#include <glm/glm.hpp>
#include <RHI.hpp>
#include <TestUtils.hpp>
#include <Window.hpp>

// clang-format off
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
// clang-format on

#include "Renderer.hpp"

/// global state
constexpr glm::vec3 g_cameraUp(0.0f, -1.0f, 0.0f);
RHI::IFramebuffer * g_defaultFramebuffer = nullptr;
std::unique_ptr<CubesRenderer> g_renderer = nullptr;
RHI::test_examples::Camera g_camera(g_cameraUp);
std::atomic_bool g_isRunning = false;

void ProcessInput(RHI::test_examples::Window & window)
{
  using namespace RHI::test_examples;

  if (window.IsKeyPressed(Keycode::KEY_ESCAPE))
    window.Close();

  if (window.IsKeyPressed(Keycode::KEY_E))
    window.SetCursorHidden(false);
  else
    window.SetCursorHidden();

  if (window.IsKeyPressed(Keycode::KEY_W))
    g_camera.MoveCamera(g_camera.GetFrontVector());
  else if (window.IsKeyPressed(Keycode::KEY_S))
    g_camera.MoveCamera(-g_camera.GetFrontVector());

  if (window.IsKeyPressed(Keycode::KEY_A))
    g_camera.MoveCamera(-g_camera.GetRightVector());
  else if (window.IsKeyPressed(Keycode::KEY_D))
    g_camera.MoveCamera(g_camera.GetRightVector());
}

// Resize window callback
//void OnResizeWindow(GLFWwindow * window, int width, int height)
//{
//  g_defaultFramebuffer->Resize(width, height);
//  g_camera.OnResolutionChanged({width, height});
//  g_renderer->InvalidateScene();
//}

//void OnCursorMoved(GLFWwindow * window, double xpos, double ypos)
//{
//  static bool st_firstCursorMovement = true;
//  static glm::dvec2 g_cursorPos{0.0};
//  if (st_firstCursorMovement)
//  {
//    g_cursorPos = {xpos, ypos};
//    st_firstCursorMovement = false;
//    return;
//  }
//
//  // reversed since y-coordinates range from bottom to top
//  g_camera.OnCursorMoved({xpos - g_cursorPos.x, ypos - g_cursorPos.y});
//  g_cursorPos = {xpos, ypos};
//}

RHI::ITexture * UploadAndCreateTexture(const char * path, RHI::IContext & ctx, bool with_alpha)
{
  int w = 0, h = 0, channels = 3;
  uint8_t * pixel_data = stbi_load(path, &w, &h, &channels, with_alpha ? STBI_rgb_alpha : STBI_rgb);
  if (!pixel_data)
  {
    throw std::runtime_error("Failed to load texture. Check it exists near the exe file");
  }

  RHI::TexelIndex extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};

  RHI::ImageCreateArguments imageArgs{};
  imageArgs.extent = extent;
  imageArgs.type = RHI::ImageType::Image2D;
  imageArgs.format = with_alpha ? RHI::ImageFormat::RGBA8 : RHI::ImageFormat::RGB8;
  imageArgs.mipLevels = 1;
  auto texture = ctx.AllocImage(imageArgs);
  texture->UploadImage(pixel_data, extent,
                       with_alpha ? RHI::HostImageFormat::RGBA8 : RHI::HostImageFormat::RGB8,
                       {{}, extent}, {{}, extent});
  stbi_image_free(pixel_data);
  return texture;
}

void game_thread_main()
{
  constexpr double st_sceneActionChance = 0.3;
  g_isRunning = true;

  {
    CubesRenderer::CubeDescription cube;
    cube.pos = {0, 0, 0};
    cube.scale = {10.0, 0.1, 10.0};
    cube.textureIndex = 0;
    g_renderer->AddCubeToScene(cube);
  }

  for (int i = 0; i < 25; ++i)
  {
    CubesRenderer::CubeDescription cube;
    cube.pos = glm::ballRand(10.0);
    cube.pos.y = glm::abs(cube.pos.y);
    cube.scale = glm::vec3(glm::gaussRand(0.5f, 0.49f));
    cube.textureIndex = 1;
    g_renderer->AddCubeToScene(cube);
  }


  while (g_isRunning)
  {
    double chanceValue = glm::linearRand(0.0, 1.0);
    if (chanceValue < st_sceneActionChance)
    {
      bool isRemove = g_renderer->GetCubesCount() > 1 &&
                      (g_renderer->GetCubesCount() >= CubesRenderer::g_MaxCubesCount ||
                       static_cast<bool>(std::rand() % 2));
      if (isRemove)
      {
        // we shouldn't remove first cube because it's floor
        size_t randCubeIdx = std::rand() % (g_renderer->GetCubesCount() - 1);
        g_renderer->DeleteCubeFromScene(randCubeIdx + 1);
      }
      else if (g_renderer->GetCubesCount() < CubesRenderer::g_MaxCubesCount)
      {
        CubesRenderer::CubeDescription cube;
        cube.pos = glm::ballRand(10.0);
        cube.axis = glm::ballRand(1.0);
        cube.pos.y = glm::abs(cube.pos.y);
        cube.scale = glm::vec3(glm::gaussRand(0.5f, 0.49f));
        cube.textureIndex = 1;
        g_renderer->AddCubeToScene(cube);
      }
    }

    g_renderer->Draw();
    std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(10));
  }
}

int main()
{
  RHI::test_examples::GlfwInstance instance;

  // Create GLFW window
  RHI::test_examples::Window window("Cubes", 800, 600);
  window.SetCursorHidden();

  g_camera.OnResolutionChanged(g_defaultWindowSize);

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  gpuTraits.require_geometry_shaders = true;
  std::unique_ptr<RHI::IContext> ctx =
    RHI::CreateContext(gpuTraits, RHI::test_examples::ConsoleLog);

  RHI::IFramebuffer * framebuffer = ctx->CreateFramebuffer();
  framebuffer->AddAttachment(2, ctx->CreateSurfacedAttachment(window.GetDrawSurface(),
                                                              RHI::RenderBuffering::Triple));
  framebuffer->AddAttachment(1,
                             ctx->AllocAttachment(RHI::ImageFormat::DEPTH_STENCIL,
                                                  {g_defaultWindowSize.x, g_defaultWindowSize.y, 1},
                                                  RHI::RenderBuffering::Triple,
                                                  RHI::SamplesCount::Eight));
  framebuffer->AddAttachment(0,
                             ctx->AllocAttachment(RHI::ImageFormat::RGBA8,
                                                  {g_defaultWindowSize.x, g_defaultWindowSize.y, 1},
                                                  RHI::RenderBuffering::Triple,
                                                  RHI::SamplesCount::Eight));

  g_renderer = std::make_unique<CubesRenderer>(*ctx);
  g_renderer->BindDrawSurface(framebuffer);
  g_renderer->BindTexture(0, UploadAndCreateTexture("floor.jpg", *ctx, false));
  g_renderer->BindTexture(1, UploadAndCreateTexture("container.png", *ctx, false));

  std::thread game_thread(game_thread_main);

  window.MainLoop(
    [&](float delta)
    {
      ProcessInput(window);
      g_renderer->SetCameraTransform(g_camera.GetVP());
      const auto start{std::chrono::steady_clock::now()};
      ctx->Flush();

      if (auto * renderTarget = framebuffer->BeginFrame())
      {
        renderTarget->SetClearValue(0, 0.1f, 0.2f, 0.4f, 1.0f);
        renderTarget->SetClearValue(1, 1.0f, 0);
        framebuffer->EndFrame();
      }

      ctx->ClearResources();
      const auto finish{std::chrono::steady_clock::now()};
      const std::chrono::duration<double, std::milli> elapsed_ms{finish - start};
      //std::printf("FPS: %.1f(%.3f)\n", 1000.0 / elapsed_ms.count(), elapsed_ms.count());
    });

  game_thread.join();
  return 0;
}
