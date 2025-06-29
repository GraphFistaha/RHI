#include "SubpassConfiguration.hpp"

#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
#include "RenderPass.hpp"
#include "Subpass.hpp"

namespace RHI::vulkan
{

SubpassConfiguration::SubpassConfiguration(Context & ctx, Subpass & owner, uint32_t subpassIndex)
  : OwnedBy<Context>(ctx)
  , OwnedBy<Subpass>(owner)
  , m_subpassIndex(subpassIndex)
  , m_descriptors(ctx, *this)
{
}

SubpassConfiguration::~SubpassConfiguration()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipeline, nullptr);
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_layout, nullptr);
}

void SubpassConfiguration::AttachShader(ShaderType type, const std::filesystem::path & path)
{
  m_pipelineBuilder.AttachShader(type, path);
  m_invalidPipeline = true;
}

void SubpassConfiguration::BindAttachment(uint32_t binding, ShaderAttachmentSlot slot)
{
  GetSubpass().GetLayout().BindAttachment(slot, binding);
  //  only colored attachment should have colorBlendState
  if (slot == ShaderAttachmentSlot::Color)
  {
    m_pipelineBuilder.OnColorAttachmentHasBound();
    m_invalidPipeline = true;
  }
  GetSubpass().GetRenderPass().SetInvalid();
}

void SubpassConfiguration::BindResolver(uint32_t binding, uint32_t resolve_for)
{
  GetSubpass().GetLayout().BindResolver(binding, resolve_for);
  GetSubpass().GetRenderPass().SetInvalid();
}

void SubpassConfiguration::AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type)
{
  m_pipelineBuilder.AddInputBinding(slot, stride, type);
  m_invalidPipeline = true;
}

void SubpassConfiguration::AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset,
                                             uint32_t elemsCount,
                                             InputAttributeElementType elemsType)
{
  m_pipelineBuilder.AddInputAttribute(binding, location, offset, elemsCount, elemsType);
  m_invalidPipeline = true;
}

IBufferUniformDescriptor * SubpassConfiguration::DeclareUniform(uint32_t binding,
                                                                ShaderType shaderStage)
{
  BufferUniform * result = nullptr;
  m_descriptors.DeclareUniformsArray(binding, shaderStage, 1, &result);
  return result;
}

ISamplerUniformDescriptor * SubpassConfiguration::DeclareSampler(uint32_t binding,
                                                                 ShaderType shaderStage)
{
  SamplerUniform * result = nullptr;
  m_descriptors.DeclareSamplersArray(binding, shaderStage, 1, &result);
  return result;
}

void SubpassConfiguration::DeclareSamplersArray(uint32_t binding, ShaderType shaderStage,
                                                uint32_t size,
                                                ISamplerUniformDescriptor * out_array[])
{
  m_descriptors.DeclareSamplersArray(binding, shaderStage, size,
                                     reinterpret_cast<SamplerUniform **>(out_array));
}

void SubpassConfiguration::DefinePushConstant(uint32_t size, ShaderType shaderStage)
{
  VkPushConstantRange newPushConstantRange{};
  newPushConstantRange.offset = 0;
  newPushConstantRange.size = size;
  newPushConstantRange.stageFlags =
    utils::CastInterfaceEnum2Vulkan<VkShaderStageFlagBits>(shaderStage);
  m_pushConstantRange.emplace(newPushConstantRange);
  m_invalidPipelineLayout = true;
}

void SubpassConfiguration::SetMeshTopology(MeshTopology topology) noexcept
{
  m_pipelineBuilder.SetMeshTopology(topology);
  m_invalidPipeline = true;
}

void SubpassConfiguration::EnableDepthTest(bool enabled) noexcept
{
  m_pipelineBuilder.SetDepthTestEnabled(enabled);
  m_invalidPipeline = true;
}

void SubpassConfiguration::SetDepthFunc(CompareOperation op) noexcept
{
  m_pipelineBuilder.SetDepthTestCompareOperator(op);
  m_invalidPipeline = true;
}

void SubpassConfiguration::SetSamplesCount(RHI::SamplesCount samplesCount) noexcept
{
  m_pipelineBuilder.SetSamplesCount(samplesCount);
  m_invalidPipeline = true;
}

void SubpassConfiguration::Invalidate()
{
  m_descriptors.Invalidate();

  if (m_invalidPipelineLayout || !m_layout)
  {
    auto new_layout =
      m_layoutBuilder.Make(GetContext().GetDevice(), m_descriptors.GetLayoutHandle(),
                           m_pushConstantRange.has_value() ? &m_pushConstantRange.value()
                                                           : nullptr);
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_layout, nullptr);
    m_layout = new_layout;
    m_invalidPipelineLayout = false;
    m_invalidPipeline = true;
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "VkPipelineLayout has been rebuilt");
  }

  if (m_invalidPipeline || !m_pipeline)
  {
    assert(CheckMSAA());

    auto new_pipeline = m_pipelineBuilder.Make(GetContext().GetDevice(),
                                               GetSubpass().GetRenderPass().GetHandle(),
                                               m_subpassIndex, m_layout);
    GetContext().Log(LogMessageStatus::LOG_DEBUG, "build new VkPipeline");
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipeline, nullptr);
    m_pipeline = new_pipeline;
    m_invalidPipeline = false;
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "VkPipeline has been rebuilt");
  }
}

void SubpassConfiguration::SetInvalid()
{
  m_invalidPipeline = true;
  m_invalidPipelineLayout = true;
}

void SubpassConfiguration::BindToCommandBuffer(const VkCommandBuffer & buffer,
                                               VkPipelineBindPoint bindPoint)
{
  assert(!!m_pipeline);
  vkCmdBindPipeline(buffer, bindPoint, m_pipeline);
  m_descriptors.BindToCommandBuffer(buffer, m_layout, bindPoint);
}

void SubpassConfiguration::TransitLayoutForUsedImages(details::CommandBuffer & commandBuffer)
{
  m_descriptors.TransitLayoutForUsedImages(commandBuffer);
}

bool SubpassConfiguration::CheckMSAA() const noexcept
{
  bool result = true;
  GetSubpass().GetLayout().ForEachAttachment(
    [this, &result](uint32_t idx)
    {
      IInternalAttachment * attachment =
        GetSubpass().GetRenderPass().GetFramebuffer().GetAttachment(idx);
      if (attachment)
      {
        result = result && attachment->GetSamplesCount() == m_pipelineBuilder.GetSamplesCount();
      }
    });
  return result;
}

} // namespace RHI::vulkan
