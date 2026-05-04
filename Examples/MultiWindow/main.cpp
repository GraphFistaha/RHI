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
  RHI::test_examples::Window window1("Multiwindow_1", 800, 600);
  RHI::test_examples::Window window2("Multiwindow_2", 800, 600);

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  RHI::IFramebuffer * framebuffer = ctx->CreateFramebuffer();
  auto * surface1 =
    ctx->CreateSurfacedAttachment(window1.GetDrawSurface(), RHI::RenderBuffering::Triple);
  auto * surface2 =
    ctx->CreateSurfacedAttachment(window2.GetDrawSurface(), RHI::RenderBuffering::Triple);
  framebuffer->AddAttachment(0, surface1);
  framebuffer->AddAttachment(1, surface2);

  auto subpass1 = framebuffer->CreateSubpass();
  {
    auto && pipeline = subpass1->GetConfiguration();
    pipeline.BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
    // set shaders
    pipeline.AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("triangle.vert")));
    pipeline.AttachShader(RHI::ShaderType::Fragment, ReadSpirV(FromGLSL("triangle_quad.frag")));
    pipeline.SetMeshTopology(RHI::MeshTopology::Triangle);
  }

  auto subpass2 = framebuffer->CreateSubpass();
  {
    auto && pipeline = subpass2->GetConfiguration();
    pipeline.BindAttachment(1, RHI::ShaderAttachmentSlot::Color);
    // set shaders
    pipeline.AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("quad.vert")));
    pipeline.AttachShader(RHI::ShaderType::Fragment, ReadSpirV(FromGLSL("triangle_quad.frag")));
    pipeline.SetMeshTopology(RHI::MeshTopology::TriangleStrip);
  }

  float t = 0.0;
  window1.MainLoop(
    [&](float delta)
    {
      surface1->SetClearValue(0.1f, std::abs(std::sin(t)), 0.4f, 1.0f);
      surface2->SetClearValue(0.1f, std::abs(std::sin(t)), 0.4f, 1.0f);
      ctx->RenderPass(framebuffer);

      if (subpass1->ShouldBeInvalidated())
      {
        auto [width, height, _] = surface1->GetDescription().extent;
        if (subpass1->BeginPass()) // begin drawing pass
        {
          subpass1->SetViewport(static_cast<float>(width), static_cast<float>(height));
          subpass1->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
          subpass1->DrawVertices(3, 1);
          subpass1->EndPass(); // finish drawing pass
        }
      }

      if (subpass2->ShouldBeInvalidated())
      {
        auto [width, height, _] = surface2->GetDescription().extent;
        if (subpass2->BeginPass()) // begin drawing pass
        {
          subpass2->SetViewport(static_cast<float>(width), static_cast<float>(height));
          subpass2->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
          subpass2->DrawVertices(4, 1);
          subpass2->EndPass(); // finish drawing pass
        }
      }
      t += 0.0001;
    });

  return 0;
}
