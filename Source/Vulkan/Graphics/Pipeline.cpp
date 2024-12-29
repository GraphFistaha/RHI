#include "Pipeline.hpp"

#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
#include "RenderPass.hpp"
#include "Subpass.hpp"

namespace RHI::vulkan
{

Pipeline::Pipeline(const Context & ctx, Subpass & owner, const RenderPass & renderPass,
                   uint32_t subpassIndex)
  : m_context(ctx)
  , m_owner(owner)
  , m_renderPass(renderPass)
  , m_subpassIndex(subpassIndex)
  , m_descriptors(ctx, *this)
{
}

Pipeline::~Pipeline()
{
  if (!!m_pipeline)
    vkDestroyPipeline(m_context.GetDevice(), m_pipeline, nullptr);
  if (!!m_layout)
    vkDestroyPipelineLayout(m_context.GetDevice(), m_layout, nullptr);
}

void Pipeline::AttachShader(ShaderType type, const std::filesystem::path & path)
{
  m_pipelineBuilder.AttachShader(type, path);
  m_invalidPipeline = true;
}

void Pipeline::AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type)
{
  m_pipelineBuilder.AddInputBinding(slot, stride, type);
  m_invalidPipeline = true;
}

void Pipeline::AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset,
                                 uint32_t elemsCount, InputAttributeElementType elemsType)
{
  m_pipelineBuilder.AddInputAttribute(binding, location, offset, elemsCount, elemsType);
  m_invalidPipeline = true;
}

IBufferUniformDescriptor * Pipeline::DeclareUniform(uint32_t binding, ShaderType shaderStage)
{
  return m_descriptors.DeclareUniform(binding, shaderStage);
}

ISamplerUniformDescriptor * Pipeline::DeclareSampler(uint32_t binding, ShaderType shaderStage)
{
  return m_descriptors.DeclareSampler(binding, shaderStage);
}

void Pipeline::DefinePushConstant(uint32_t size, ShaderType shaderStage)
{
  VkPushConstantRange newPushConstantRange{};
  newPushConstantRange.offset = 0;
  newPushConstantRange.size = size;
  newPushConstantRange.stageFlags =
    utils::CastInterfaceEnum2Vulkan<VkShaderStageFlagBits>(shaderStage);
  m_pushConstantRange.emplace(newPushConstantRange);
  m_invalidPipelineLayout = true;
}

void Pipeline::Invalidate()
{
  m_descriptors.Invalidate();

  if (m_invalidPipelineLayout || !m_layout)
  {
    auto new_layout = m_layoutBuilder.Make(m_context.GetDevice(), m_descriptors.GetLayoutHandle(),
                                           m_pushConstantRange.has_value()
                                             ? &m_pushConstantRange.value()
                                             : nullptr);
    if (!!m_layout)
      vkDestroyPipelineLayout(m_context.GetDevice(), m_layout, nullptr);
    m_layout = new_layout;
    m_invalidPipelineLayout = false;
    m_invalidPipeline = true;
  }

  if (m_invalidPipeline || !m_pipeline)
  {
    auto new_pipeline = m_pipelineBuilder.Make(m_context.GetDevice(), m_renderPass.GetHandle(),
                                               m_subpassIndex, m_layout);
    m_context.Log(LogMessageStatus::LOG_DEBUG, "build new VkPipeline");
    if (!!m_pipeline)
      vkDestroyPipeline(m_context.GetDevice(), m_pipeline, nullptr);
    m_pipeline = new_pipeline;
    m_invalidPipeline = false;
  }

  m_owner.SetDirtyCacheCommands();
}

void Pipeline::BindToCommandBuffer(const vk::CommandBuffer & buffer, VkPipelineBindPoint bindPoint)
{
  assert(m_pipeline);
  vkCmdBindPipeline(buffer, bindPoint, m_pipeline);
  m_descriptors.BindToCommandBuffer(buffer, m_layout, bindPoint);
}

} // namespace RHI::vulkan
