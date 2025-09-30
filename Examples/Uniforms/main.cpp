#include <cmath>
#include <cstdio>
#include <cstring>

#include <RHI.hpp>
#include <TestUtils.hpp>
#include <Window.hpp>

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
  RHI::test_examples::GlfwInstance instance;

  RHI::test_examples::Window window("Uniforms", 800, 600);

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  // create buffers for each uniform variables
  auto tBuf = ctx->AllocBuffer(sizeof(float), RHI::BufferGPUUsage::UniformBuffer, true);
  auto transformBuf = ctx->AllocBuffer(2 * sizeof(float), RHI::BufferGPUUsage::UniformBuffer, true);

  auto * framebuffer = ctx->CreateFramebuffer();
  framebuffer->AddAttachment(0, ctx->CreateSurfacedAttachment(window.GetDrawSurface(),
                                                              RHI::RenderBuffering::Triple));

  auto * subpass = framebuffer->CreateSubpass();
  auto && trianglePipeline = subpass->GetConfiguration();
  trianglePipeline.BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
  trianglePipeline.AttachShader(RHI::ShaderType::Vertex, "uniform.vert");
  trianglePipeline.AttachShader(RHI::ShaderType::Fragment, "uniform.frag");
  // set vertex attributes (5 float attributes per vertex - pos.xy and color.rgb)
  trianglePipeline.AddInputBinding(0, 5 * sizeof(float), RHI::InputBindingType::VertexData);
  trianglePipeline.AddInputAttribute(0, 0, 0, 2, RHI::InputAttributeElementType::FLOAT);
  trianglePipeline.AddInputAttribute(0, 1, 2 * sizeof(float), 3,
                                     RHI::InputAttributeElementType::FLOAT);

  // declare uniform variables
  auto && u_t =
    trianglePipeline.DeclareUniform({0, 0}, RHI::ShaderType::Fragment | RHI::ShaderType::Vertex);
  u_t->AssignBuffer(*tBuf); // bind buffer to uniform variable

  auto && u_transform = trianglePipeline.DeclareUniform({0, 1}, RHI::ShaderType::Vertex);
  u_transform->AssignBuffer(*transformBuf); // bind buffer to uniform variable

  // create vertex buffer
  auto && vertexBuffer =
    ctx->AllocBuffer(VerticesCount * 5 * sizeof(float), RHI::BufferGPUUsage::VertexBuffer, false);
  vertexBuffer->UploadAsync(Vertices, VerticesCount * 5 * sizeof(float));

  // create index buffer
  auto indexBuffer =
    ctx->AllocBuffer(IndicesCount * sizeof(uint32_t), RHI::BufferGPUUsage::IndexBuffer, false);
  indexBuffer->UploadAsync(Indices, IndicesCount * sizeof(uint32_t));

  window.onResize = [framebuffer](int width, int height)
  {
    framebuffer->Resize(width, height);
  };

  float x = 0.0f;
  window.MainLoop(
    [&](float delta)
    {
      float t_val = std::abs(std::sin(x));
      tBuf->UploadSync(&t_val, sizeof(float));

      std::pair<float, float> transform_val{std::cos(x), std::sin(x)};
      transformBuf->UploadAsync(&transform_val, 2 * sizeof(float));

      x += 0.001f;
      ctx->Flush();

      if (RHI::IRenderTarget * renderTarget = framebuffer->BeginFrame())
      {
        renderTarget->SetClearValue(0, 0.3f, 0.3f, 0.5f, 1.0f);
        if (subpass->ShouldBeInvalidated())
        {
          // get size of window
          auto [width, height, _] = renderTarget->GetExtent();
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
        }

        framebuffer->EndFrame();
      }

      ctx->ClearResources();
    });

  return 0;
}
