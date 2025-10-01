#include "Renderer.hpp"

#include <glm/ext.hpp>
#include <TestUtils.hpp>

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
    subpassConfig.BindResolver(2, 0);
    subpassConfig.EnableDepthTest(true);
    // set shaders
    subpassConfig.AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("cube.vert")));
    subpassConfig.AttachShader(RHI::ShaderType::Geometry, ReadSpirV(FromGLSL("cube.geom")));
    subpassConfig.AttachShader(RHI::ShaderType::Fragment, ReadSpirV(FromGLSL("cube.frag")));
    subpassConfig.SetMeshTopology(RHI::MeshTopology::Point);

    subpassConfig.AddInputBinding(0, sizeof(CubeDescription), RHI::InputBindingType::VertexData);
    subpassConfig.AddInputAttribute(0, 0, 0, 3, RHI::InputAttributeElementType::FLOAT);
    subpassConfig.AddInputAttribute(0, 1, 3 * sizeof(float), 3,
                                    RHI::InputAttributeElementType::FLOAT);
    subpassConfig.AddInputAttribute(0, 2, 6 * sizeof(float), 3,
                                    RHI::InputAttributeElementType::FLOAT);
    subpassConfig.AddInputAttribute(0, 3, 9 * sizeof(float), 1,
                                    RHI::InputAttributeElementType::SINT);


    auto * uniform = subpassConfig.DeclareUniform({0, 0}, RHI::ShaderType::Vertex);
    uniform->AssignBuffer(*m_uniformBuffer);
    subpassConfig.DeclareSamplersArray({0, 1}, RHI::ShaderType::Fragment,
                                       static_cast<uint32_t>(m_textures.size()), m_textures.data());
    for (auto * texture : m_textures)
      texture->SetFilter(RHI::TextureFilteration::Linear, RHI::TextureFilteration::Linear);
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

void CubesRenderer::BindTexture(uint32_t idx, RHI::ITexture * texture)
{
  if (m_textures[idx])
    m_textures[idx]->AssignImage(texture);
}

void CubesRenderer::Draw()
{
  if (m_invalidGeometry && m_cubesCount > 0)
  {
    m_cubesBuffer->UploadAsync(m_cubes.data(), m_cubesCount * sizeof(CubeDescription));
    m_invalidGeometry = false;
    m_invalidScene = true;
  }

  if (m_invalidScene || m_renderPass->ShouldBeInvalidated())
  {
    m_renderPass->BeginPass();
    auto extent = m_drawSurface->GetExtent();
    uint32_t width = extent[0], height = extent[1];
    m_renderPass->SetViewport(static_cast<float>(width), static_cast<float>(height));
    m_renderPass->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    m_renderPass->BindVertexBuffer(0, *m_cubesBuffer);
    m_renderPass->DrawVertices(static_cast<uint32_t>(m_cubesCount), 1);

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

CubesRenderer::UniformBlock::UniformBlock()
  : vp(glm::identity<glm::mat4>())
{
}
