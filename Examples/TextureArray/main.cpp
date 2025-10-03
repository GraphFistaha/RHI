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
  uint32_t texture_index;
};


int main()
{
  RHI::test_examples::GlfwInstance instance;
  RHI::test_examples::Window window("TextureArray", 800, 600);

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  std::vector<RHI::ITexture *> textures;
  textures.emplace_back(UploadTexture("BT_texture.png", ctx.get(), true));
  textures.emplace_back(UploadTexture("BT_jackal.jpg", ctx.get(), false));
  textures.emplace_back(UploadTexture("BT_shrek1.jpg", ctx.get(), false));
  textures.emplace_back(UploadTexture("BT_shrek2.jpg", ctx.get(), false));
  textures.emplace_back(UploadTexture("BT_mike_wazowski.jpg", ctx.get(), false));
  textures.emplace_back(UploadTexture("BT_pepe.jpg", ctx.get(), false));
  auto image_it = textures.begin();

  RHI::IFramebuffer * framebuffer = ctx->CreateFramebuffer();
  framebuffer->AddAttachment(0, ctx->CreateSurfacedAttachment(window.GetDrawSurface(),
                                                              RHI::RenderBuffering::Triple));
  auto * subpass = framebuffer->CreateSubpass();
  // create pipeline for triangle. Here we can configure gpu pipeline for rendering
  auto && trianglePipeline = subpass->GetConfiguration();
  trianglePipeline.BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
  trianglePipeline.AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("texture_array.vert")));
  trianglePipeline.AttachShader(RHI::ShaderType::Fragment,
                                ReadSpirV(FromGLSL("texture_array.frag")));
  trianglePipeline.DefinePushConstant(sizeof(PushConstant),
                                      RHI::ShaderType::Fragment | RHI::ShaderType::Vertex);
  constexpr uint32_t samplersCount = 8;
  std::array<RHI::ISamplerUniformDescriptor *, samplersCount> samplers;
  trianglePipeline.DeclareSamplersArray({0, 0}, RHI::ShaderType::Fragment, samplersCount,
                                        samplers.data());

  {
    auto it = image_it;
    for (auto && sampler : samplers)
    {
      sampler->AssignImage(*it);
      it = std::next(it);
      if (it == textures.end())
        it = textures.begin();
    }
  }

  window.MainLoop(
    [&](float delta)
    {
      ctx->TransferPass();

      if (window.IsKeyPressed(RHI::test_examples::Keycode::KEY_ENTER))
      {
        image_it = std::next(image_it);
        if (image_it == textures.end())
          image_it = textures.begin();
        auto it = image_it;
        for (auto && sampler : samplers)
        {
          sampler->AssignImage(*it);
          it = std::next(it);
          if (it == textures.end())
            it = textures.begin();
        }
      }

      if (RHI::IRenderTarget * renderTarget = framebuffer->BeginFrame())
      {
        renderTarget->SetClearValue(0, 0.3f, 0.3f, 0.5f, 1.0f);
        if (subpass->ShouldBeInvalidated())
        {
          auto [width, height, _] = renderTarget->GetExtent();
          subpass->BeginPass();
          subpass->SetViewport(static_cast<float>(width), static_cast<float>(height));
          subpass->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
          constexpr int cells_count_in_row = 8;
          constexpr float cell_width = 2.0f / cells_count_in_row;
          constexpr float offset = -1.0f + cell_width / 2.0f;
          constexpr float margin = 0.05f;
          PushConstant ct;
          ct.scale_x = cell_width / 2.0f;
          ct.scale_y = cell_width / 2.0f;
          ct.texture_index = 0;
          for (int i = 0; i <= cells_count_in_row; ++i)
          {
            for (int j = 0; j <= cells_count_in_row; ++j)
            {
              ct.pos_x = offset + j * (cell_width + margin);
              ct.pos_y = offset + i * (cell_width + margin);
              ct.texture_index = (ct.texture_index + 1) % textures.size();
              subpass->PushConstant(&ct, sizeof(PushConstant));
              subpass->DrawVertices(6, 1);
            }
          }
          subpass->EndPass();
        }

        framebuffer->EndFrame();
      }
    });

  return 0;
}
