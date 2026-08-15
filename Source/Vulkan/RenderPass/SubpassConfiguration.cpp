#include "SubpassConfiguration.hpp"

#include <Descriptors/InputAttachmentUniform.hpp>
#include <RenderPass/Framebuffer.hpp>
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
{
}

SubpassConfiguration::~SubpassConfiguration()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipeline, nullptr);
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipelineLayout, nullptr);
}

void SubpassConfiguration::AttachShader(ShaderType type, const SpirV & spirv)
{
  m_pipelineBuilder.AttachShader(type, spirv);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
}

void SubpassConfiguration::BindAttachment(uint32_t binding, ShaderAttachmentSlot slot,
                                          LayoutIndex inputIndex /* = LayoutIndex()*/)
{
  GetSubpass().GetLayout().BindAttachment(slot, binding);
  //  only colored attachment should have colorBlendState
  if (slot & ShaderAttachmentSlot::Color)
  {
    m_pipelineBuilder.OnColorAttachmentHasBound();
    m_invalidPipeline = true;
    m_invalidPipeline.notify_one();
  }
  if (slot & ShaderAttachmentSlot::Input)
  {
    if (!inputIndex.IsValid())
    {
      throw std::runtime_error(
        "Input attachments require valid layout index. Don't forget to declare this uniform in fragment shader");
    }
    GetSubpass().GetDescriptorBuffer().GetLayout().DeclareInputAttachmentUniform(
      inputIndex, RHI::ShaderType::Fragment); // input attachments are fragment-shader-only feature
    InputAttachmentUniform uniform(GetContext(), GetSubpass().GetDescriptorBuffer().GetLayout(),
                                   inputIndex);
    GetSubpass().OnDescriptorChanged(uniform.CreateUpdateTask());
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
  GetSubpass().GetDescriptorsLayout().DeclareBufferUniformsArray(index, shaderStage, 1, &result);
  return result;
}

ISamplerUniformDescriptor * SubpassConfiguration::DeclareSampler(LayoutIndex index,
                                                                 ShaderType shaderStage)
{
  ISamplerUniformDescriptor * result = nullptr;
  GetSubpass().GetDescriptorsLayout().DeclareSamplerUniformsArray(index, shaderStage, 1, &result);
  return result;
}

void SubpassConfiguration::DeclareUniformsArray(LayoutIndex index, ShaderType shaderStage,
                                                uint32_t size,
                                                IBufferUniformDescriptor * outArray[])
{
    GetSubpass().GetDescriptorsLayout().DeclareBufferUniformsArray(index, shaderStage, size, outArray);
}

void SubpassConfiguration::DeclareSamplersArray(LayoutIndex index, ShaderType shaderStage,
                                                uint32_t size,
                                                ISamplerUniformDescriptor * outArray[])
{
    GetSubpass().GetDescriptorsLayout().DeclareSamplerUniformsArray(index, shaderStage, size, outArray);
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

void SubpassConfiguration::SetRenderProcess(PipelineProcessPtr process)
{
  GetSubpass().SetRenderProcess(std::move(process));
}

void SubpassConfiguration::Invalidate()
{
  if (m_invalidPipelineLayout || !m_pipelineLayout)
  {
    auto && layoutHandles = GetSubpass().GetDescriptorsLayout().GetHandles();
    auto new_layout = m_pipelineLayoutBuilder.Make(GetContext().GetGpuConnection().GetDevice(),
                                                   layoutHandles.data(),
                                                   static_cast<uint32_t>(layoutHandles.size()),
                                                   m_pushConstantRange.has_value()
                                                     ? &m_pushConstantRange.value()
                                                     : nullptr);
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipelineLayout, nullptr);
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "VkPipelineLayout({}) has been rebuilt - {}",
                     static_cast<void *>(m_pipelineLayout), static_cast<void *>(new_layout));
    m_pipelineLayout = new_layout;
    m_invalidPipelineLayout = false;
    m_invalidPipeline = true;
  }

  if (m_invalidPipeline || !m_pipeline)
  {
    m_pipelineBuilder.SetSamplesCount(
      GetSubpass().GetRenderPass().GetFramebuffer().CalcSamplesCount());
    auto new_pipeline = m_pipelineBuilder.Make(GetContext().GetGpuConnection().GetDevice(),
                                               GetSubpass().GetRenderPass().GetHandle(),
                                               m_subpassIndex, m_pipelineLayout);
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipeline, nullptr);
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "VkPipeline({}) has been rebuilt - {}",
                     static_cast<void *>(m_pipeline), static_cast<void *>(new_pipeline));
    m_pipeline = new_pipeline;
    m_invalidPipeline = false;
    m_invalidPipeline.notify_one();
    GetSubpass().SetDirtyCommands();
  }
}

void SubpassConfiguration::SetInvalid()
{
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
  m_invalidPipelineLayout = true;
}

bool SubpassConfiguration::WaitForPipelineIsValid() const noexcept
{
  bool result = false;
  for (int i = 0; i < 1000; ++i)
  {
    if (m_invalidPipeline == false)
      return true;
  }
  return false;
}

void SubpassConfiguration::BindToCommandBuffer(details::CommandBuffer & commands,
                                               VkPipelineBindPoint bindPoint)
{
  assert(!!m_pipeline);
  commands.PushCommand(vkCmdBindPipeline, bindPoint, m_pipeline);
}

} // namespace RHI::vulkan
