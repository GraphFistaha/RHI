#include "SubpassConfiguration.hpp"

#include <RenderPass/RenderPass.hpp>
#include <RenderPass/Subpass.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{

SubpassConfiguration::SubpassConfiguration(Context & ctx, Subpass & owner, uint32_t subpassIndex)
  : OwnedBy<Context>(ctx)
  , OwnedBy<Subpass>(owner)
  , m_subpassIndex(subpassIndex)
  , m_descriptorsLayout(ctx, *this)
{
}

SubpassConfiguration::~SubpassConfiguration()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipeline, nullptr);
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipelineLayout, nullptr);
}

void SubpassConfiguration::AttachShader(ShaderType type, const std::filesystem::path & path)
{
  m_pipelineBuilder.AttachShader(type, path);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
}

void SubpassConfiguration::BindAttachment(uint32_t binding, ShaderAttachmentSlot slot)
{
  GetSubpass().GetLayout().BindAttachment(slot, binding);
  //  only colored attachment should have colorBlendState
  if (slot == ShaderAttachmentSlot::Color)
  {
    m_pipelineBuilder.OnColorAttachmentHasBound();
    m_invalidPipeline = true;
    m_invalidPipeline.notify_one();
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
  m_invalidPipeline.notify_one();
}

void SubpassConfiguration::AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset,
                                             uint32_t elemsCount,
                                             InputAttributeElementType elemsType)
{
  m_pipelineBuilder.AddInputAttribute(binding, location, offset, elemsCount, elemsType);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
}

IBufferUniformDescriptor * SubpassConfiguration::DeclareUniform(LayoutIndex index,
                                                                ShaderType shaderStage)
{
  IBufferUniformDescriptor * result = nullptr;
  m_descriptorsLayout.DeclareBufferUniformsArray(index, shaderStage, 1, &result);
  return result;
}

ISamplerUniformDescriptor * SubpassConfiguration::DeclareSampler(LayoutIndex index,
                                                                 ShaderType shaderStage)
{
  ISamplerUniformDescriptor * result = nullptr;
  m_descriptorsLayout.DeclareSamplerUniformsArray(index, shaderStage, 1, &result);
  return result;
}

void SubpassConfiguration::DeclareSamplersArray(LayoutIndex index, ShaderType shaderStage,
                                                uint32_t size,
                                                ISamplerUniformDescriptor * out_array[])
{
  m_descriptorsLayout.DeclareSamplerUniformsArray(index, shaderStage, size, out_array);
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
  m_invalidPipeline.notify_one();
}

void SubpassConfiguration::EnableDepthTest(bool enabled) noexcept
{
  m_pipelineBuilder.SetDepthTestEnabled(enabled);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
  GetSubpass().SetInvalid();
}

void SubpassConfiguration::SetDepthFunc(CompareOperation op) noexcept
{
  m_pipelineBuilder.SetDepthTestCompareOperator(op);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
}

void SubpassConfiguration::Invalidate()
{
  m_descriptorsLayout.Invalidate();

  if (m_invalidPipelineLayout || !m_pipelineLayout)
  {
    auto && layoutHandles = m_descriptorsLayout.GetHandles();
    auto new_layout = m_pipelineLayoutBuilder.Make(GetContext().GetDevice(), layoutHandles.data(),
                                                   static_cast<uint32_t>(layoutHandles.size()),
                                                   m_pushConstantRange.has_value()
                                                     ? &m_pushConstantRange.value()
                                                     : nullptr);
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipelineLayout, nullptr);
    m_pipelineLayout = new_layout;
    m_invalidPipelineLayout = false;
    m_invalidPipeline = true;
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "VkPipelineLayout has been rebuilt");
  }

  if (m_invalidPipeline || !m_pipeline)
  {
    m_pipelineBuilder.SetSamplesCount(
      GetSubpass().GetRenderPass().GetFramebuffer().CalcSamplesCount());
    auto new_pipeline = m_pipelineBuilder.Make(GetContext().GetDevice(),
                                               GetSubpass().GetRenderPass().GetHandle(),
                                               m_subpassIndex, m_pipelineLayout);
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipeline, nullptr);
    m_pipeline = new_pipeline;
    m_invalidPipeline = false;
    m_invalidPipeline.notify_one();
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "VkPipeline has been rebuilt");
    GetSubpass().SetDirtyCacheCommands();
  }
}

void SubpassConfiguration::SetInvalid()
{
  m_descriptorsLayout.SetInvalid();
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
  m_invalidPipelineLayout = true;
}

void SubpassConfiguration::WaitForPipelineIsValid() const noexcept
{
  std::atomic_wait(&m_invalidPipeline, true);
}

const DescriptorBufferLayout & SubpassConfiguration::GetDescriptorsLayout() const & noexcept
{
  return m_descriptorsLayout;
}

void SubpassConfiguration::BindToCommandBuffer(const VkCommandBuffer & buffer,
                                               VkPipelineBindPoint bindPoint)
{
  assert(!!m_pipeline);
  vkCmdBindPipeline(buffer, bindPoint, m_pipeline);
}

void SubpassConfiguration::TransitLayoutForUsedImages(details::CommandBuffer & commandBuffer)
{
  m_descriptorsLayout.TransitLayoutForUsedImages(commandBuffer);
}

} // namespace RHI::vulkan
