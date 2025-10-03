#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <future>

#include <RHI.hpp>
#include <TestUtils.hpp>
#include <Window.hpp>

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
  explicit Renderer(RHI::IContext & ctx, RHI::IFramebuffer & framebuffer);
  ~Renderer();
  // draw scene in parallel
  void AsyncDrawScene();

  void UpdateGeometry();

private:
  RHI::IFramebuffer * m_fbo;
  /// subpass which can be executed in parallel
  RHI::ISubpass * m_subpass;

  /// some data for frame
  RHI::IBufferGPU * m_vertexBuffer;
  RHI::IBufferGPU * m_indexBuffer;

  /// current draw task
  std::future<bool> m_drawTask;

private:
  bool DrawSceneImpl();
};

int main()
{
  RHI::test_examples::GlfwInstance instance;
  RHI::test_examples::Window window("ParallelPass", 800, 600);

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  RHI::IFramebuffer * framebuffer = ctx->CreateFramebuffer();
  framebuffer->AddAttachment(0, ctx->CreateSurfacedAttachment(window.GetDrawSurface(),
                                                              RHI::RenderBuffering::Triple));

  Renderer triangleRenderer(*ctx, *framebuffer);
  triangleRenderer.AsyncDrawScene();

  window.onResize = [&triangleRenderer, &framebuffer](int width, int height)
  {
    framebuffer->Resize(width, height);
    triangleRenderer.AsyncDrawScene();
  };

  window.MainLoop(
    [framebuffer, &ctx, &triangleRenderer](float delta)
    {
      triangleRenderer.UpdateGeometry();
      ctx->TransferPass();
      if (auto * renderTarget = framebuffer->BeginFrame())
      {
        renderTarget->SetClearValue(0, 0.1f, 1.0f, 0.4f, 1.0f);
        framebuffer->EndFrame();
      }
    });

  return 0;
}

Renderer::Renderer(RHI::IContext & ctx, RHI::IFramebuffer & framebuffer)
  : m_fbo(&framebuffer)
{
  // create pipeline for triangle. Here we can configure gpu pipeline for rendering
  m_subpass = framebuffer.CreateSubpass();
  auto && trianglePipeline = m_subpass->GetConfiguration();
  trianglePipeline.BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
  // set shaders
  trianglePipeline.AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("triangle.vert")));
  trianglePipeline.AttachShader(RHI::ShaderType::Fragment, ReadSpirV(FromGLSL("triangle.frag")));
  // set vertex attributes (5 float attributes per vertex - pos.xy and color.rgb)
  trianglePipeline.AddInputBinding(0, 5 * sizeof(float), RHI::InputBindingType::VertexData);
  trianglePipeline.AddInputAttribute(0, 0, 0, 2, RHI::InputAttributeElementType::FLOAT);
  trianglePipeline.AddInputAttribute(0, 1, 2 * sizeof(float), 3,
                                     RHI::InputAttributeElementType::FLOAT);

  // create vertex buffer
  m_vertexBuffer =
    ctx.AllocBuffer(VerticesCount * 5 * sizeof(float), RHI::BufferGPUUsage::VertexBuffer, false);
  m_vertexBuffer->UploadAsync(Vertices, VerticesCount * 5 * sizeof(float));


  // create index buffer
  m_indexBuffer =
    ctx.AllocBuffer(IndicesCount * sizeof(uint32_t), RHI::BufferGPUUsage::IndexBuffer, false);
  m_indexBuffer->UploadAsync(Indices, IndicesCount * sizeof(uint32_t));
}

Renderer::~Renderer()
{
  //TODO: destroy buffers
}

void Renderer::AsyncDrawScene()
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

void Renderer::UpdateGeometry()
{
  m_vertexBuffer->UploadAsync(Vertices, VerticesCount * 5 * sizeof(float));
  m_indexBuffer->UploadAsync(Indices, IndicesCount * sizeof(uint32_t));
  AsyncDrawScene();
}

bool Renderer::DrawSceneImpl()
{
  if (m_subpass->ShouldBeInvalidated())
  {
    auto extent = m_fbo->GetExtent();
    m_subpass->BeginPass();
    m_subpass->SetViewport(static_cast<float>(extent[0]), static_cast<float>(extent[1]));
    m_subpass->SetScissor(0, 0, static_cast<uint32_t>(extent[0]), static_cast<uint32_t>(extent[1]));
    m_subpass->BindVertexBuffer(0, *m_vertexBuffer, 0);
    m_subpass->BindIndexBuffer(*m_indexBuffer, RHI::IndexType::UINT32);
    m_subpass->DrawIndexedVertices(IndicesCount, 1);
    m_subpass->EndPass();
  }
  return true;
}
