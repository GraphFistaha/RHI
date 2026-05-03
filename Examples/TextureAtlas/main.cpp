#include <cmath>
#include <cstdio>
#include <cstring>

#include <RHI.hpp>
#include <TestUtils.hpp>
#include <Window.hpp>

struct PushConstant
{
  float scale_x;
  float scale_y;
  float pos_x;
  float pos_y;
};

int main()
{
  RHI::test_examples::GlfwInstance instance;
  RHI::test_examples::Window window("TextureAtlas", 800, 600);

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  RHI::ITexture * texture = BuildAtlasTexture(ctx.get(), 2048,
                                              {"texture.png", "jackal.jpg", "shrek1.jpg",
                                               "shrek2.jpg", "mike_wazowski.jpg", "pepe.jpg"},
                                              true, true);


  RHI::IFramebuffer * framebuffer = ctx->CreateFramebuffer();
  framebuffer->AddAttachment(0, ctx->CreateSurfacedAttachment(window.GetDrawSurface(),
                                                              RHI::RenderBuffering::Triple));

  window.onResize = [framebuffer](int width, int height)
  {
    framebuffer->Resize(width, height);
  };

  auto * subpass = framebuffer->CreateSubpass();
  // create pipeline for triangle. Here we can configure gpu pipeline for rendering
  auto && trianglePipeline = subpass->GetConfiguration();
  trianglePipeline.BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
  trianglePipeline.AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("textureAtlas.vert")));
  trianglePipeline.AttachShader(RHI::ShaderType::Fragment,
                                ReadSpirV(FromGLSL("textureAtlas.frag")));
  trianglePipeline.DefinePushConstant(sizeof(PushConstant),
                                      RHI::ShaderType::Fragment | RHI::ShaderType::Vertex);

  RHI::ISamplerUniformDescriptor * sampler;
  {
    auto * sampler = trianglePipeline.DeclareSampler({0, 0}, RHI::ShaderType::Fragment);
    sampler->SetFilter(RHI::TextureFilteration::Linear, RHI::TextureFilteration::Linear);
    sampler->AssignImage(texture);
  }

  window.MainLoop(
    [&](float delta)
    {
      ctx->TransferPass();

      if (RHI::IRenderTarget * renderTarget = framebuffer->BeginFrame())
      {
        renderTarget->SetClearValue(0, 0.3f, 0.3f, 0.5f, 1.0f);

        if (subpass->ShouldBeInvalidated())
        {
          auto [width, height, depth] = renderTarget->GetExtent();
          if (subpass->BeginPass())
          {
            subpass->SetViewport(static_cast<float>(width), static_cast<float>(height));
            subpass->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            subpass->DrawVertices(6, 1);

            subpass->EndPass();
          }
        }
        framebuffer->EndFrame();
      }
    });

  return 0;
}
