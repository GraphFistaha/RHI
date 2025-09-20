#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <RHI.hpp>
#include <TestUtils.hpp>
#include <Window.hpp>

int main()
{
  RHI::test_examples::GlfwInstance instance;

  RHI::test_examples::Window window("Hello window", 800, 600);

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx =
    RHI::CreateContext(gpuTraits, RHI::test_examples::ConsoleLog);
  assert(ctx);

  RHI::IFramebuffer * framebuffer = ctx->CreateFramebuffer();
  framebuffer->AddAttachment(0, ctx->CreateSurfacedAttachment(window.GetDrawSurface(),
                                                              RHI::RenderBuffering::Double));

  window.onResize = [&framebuffer](int width, int height)
  {
    framebuffer->Resize(width, height);
  };

  float t = 0.0;
  window.MainLoop(
    [=, &t](float delta)
    {
      if (RHI::IRenderTarget * renderTarget = framebuffer->BeginFrame())
      {
        renderTarget->SetClearValue(0, 0.1f, std::abs(std::sin(t)), 0.4f, 1.0f);
        framebuffer->EndFrame();
      }
      t += 0.001f;
    });

  return 0;
}
