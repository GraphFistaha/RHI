#include "Pipeline.hpp"

#include <Descriptors/DescriptorBufferLayout.hpp>
#include <Descriptors/InputAttachmentUniform.hpp>
#include <RenderPass/Framebuffer.hpp>
#include <RenderPass/PipelineProcess.hpp>
#include <RenderPass/RenderPass.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{

Pipeline::Pipeline(Context & ctx, RenderPass & owner, uint32_t subpassIndex,
                                           uint32_t familyQueue)
  : OwnedBy<Context>(ctx)
  , OwnedBy<RenderPass>(owner)
  , m_subpassIndex(subpassIndex)
  , m_execBuffer(ctx, familyQueue, VK_COMMAND_BUFFER_LEVEL_SECONDARY)
  , m_writeBuffer(ctx, familyQueue, VK_COMMAND_BUFFER_LEVEL_SECONDARY)
  , m_descriptorsLayout(ctx, *this)
  , m_execDescriptorBuffer(ctx, m_descriptorsLayout)
  , m_writeDescriptorBuffer(ctx, m_descriptorsLayout)
{
}

Pipeline::~Pipeline()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipeline, nullptr);
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipelineLayout, nullptr);
}

void Pipeline::AttachShader(ShaderType type, const SpirV & spirv)
{
  m_pipelineBuilder.AttachShader(type, spirv);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
}

void Pipeline::BindAttachment(uint32_t binding, ShaderAttachmentSlot slot,
                                          LayoutIndex inputIndex /* = LayoutIndex()*/)
{
  GetLayout().BindAttachment(slot, binding);
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
    GetDescriptorBuffer().GetLayout().DeclareInputAttachmentUniform(
      inputIndex, RHI::ShaderType::Fragment); // input attachments are fragment-shader-only feature
    InputAttachmentUniform uniform(GetContext(), GetDescriptorBuffer().GetLayout(), inputIndex);
    OnDescriptorChanged(uniform.CreateUpdateTask());
  }
  GetRenderPass().SetInvalid();
}

void Pipeline::BindResolver(uint32_t binding, uint32_t resolve_for)
{
  GetLayout().BindResolver(binding, resolve_for);
  GetRenderPass().SetInvalid();
}

void Pipeline::AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type)
{
  m_pipelineBuilder.AddInputBinding(slot, stride, type);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
}

void Pipeline::AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset,
                                             uint32_t elemsCount,
                                             InputAttributeElementType elemsType)
{
  m_pipelineBuilder.AddInputAttribute(binding, location, offset, elemsCount, elemsType);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
}

IBufferUniformDescriptor * Pipeline::DeclareUniform(LayoutIndex index,
                                                                ShaderType shaderStage)
{
  IBufferUniformDescriptor * result = nullptr;
  GetDescriptorsLayout().DeclareBufferUniformsArray(index, shaderStage, 1, &result);
  return result;
}

ISamplerUniformDescriptor * Pipeline::DeclareSampler(LayoutIndex index,
                                                                 ShaderType shaderStage)
{
  ISamplerUniformDescriptor * result = nullptr;
  GetDescriptorsLayout().DeclareSamplerUniformsArray(index, shaderStage, 1, &result);
  return result;
}

void Pipeline::DeclareUniformsArray(LayoutIndex index, ShaderType shaderStage,
                                                uint32_t size,
                                                IBufferUniformDescriptor * outArray[])
{
  GetDescriptorsLayout().DeclareBufferUniformsArray(index, shaderStage, size, outArray);
}

void Pipeline::DeclareSamplersArray(LayoutIndex index, ShaderType shaderStage,
                                                uint32_t size,
                                                ISamplerUniformDescriptor * outArray[])
{
  GetDescriptorsLayout().DeclareSamplerUniformsArray(index, shaderStage, size, outArray);
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

void Pipeline::SetMeshTopology(MeshTopology topology) noexcept
{
  m_pipelineBuilder.SetMeshTopology(topology);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
}

void Pipeline::EnableDepthTest(bool enabled) noexcept
{
  m_pipelineBuilder.SetDepthTestEnabled(enabled);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
  SetInvalid();
}

void Pipeline::SetDepthFunc(CompareOperation op) noexcept
{
  m_pipelineBuilder.SetDepthTestCompareOperator(op);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
}

void Pipeline::SetRenderProcess(PipelineProcessPtr process)
{
  m_process = FastDynamicCast<PipelineProcess>(process);
  if (m_process)
    m_process->CommitProcess();
  SetDirtyCommands();
}

void Pipeline::RecordCommands(details::CommandBuffer & commands)
{
  GetRenderPass().WaitForRenderPassIsValid(); // wait for render pass is valid
  if (m_dirtyCommands)
  {
    m_writeBuffer.Reset();
    m_writeBuffer.BeginWriting(GetRenderPass().GetHandle(), GetSubpassIndex());
    BindToCommandBuffer(m_writeBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
    m_writeDescriptorBuffer.BindToCommandBuffer(m_writeBuffer, GetPipelineLayoutHandle(),
                                                VK_PIPELINE_BIND_POINT_GRAPHICS);
    if (m_process)
      m_process->RecordCommands(m_writeBuffer, *this);

    m_writeBuffer.EndWriting();
    std::swap(m_writeBuffer, m_execBuffer);
    m_dirtyCommands = false;
  }

  commands.AddCommands(m_execBuffer.GetHandle());
}

void Pipeline::CollectResources(std::vector<ResourcePtr> & resources) const
{
  // collect descriptors and uniforms
  m_descriptorsLayout.CollectResources(resources);
  // collect resources from draw commands (vertex/index buffers)
  if (m_process)
    m_process->CollectResources(resources);
}

void Pipeline::SynchroniseResources(details::CommandBuffer & commands) const
{
  // collect descriptors and uniforms
  m_descriptorsLayout.SynchroniseResources(commands);
  // collect resources from draw commands (vertex/index buffers)
  if (m_process)
    m_process->SynchroniseResources(commands);
}

void Pipeline::Invalidate()
{
  m_descriptorsLayout.Invalidate();
  m_execDescriptorBuffer.Invalidate();
  m_writeDescriptorBuffer.Invalidate();
  if (m_invalidPipelineLayout || !m_pipelineLayout)
  {
    auto && layoutHandles = GetDescriptorsLayout().GetHandles();
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
    m_pipelineBuilder.SetSamplesCount(GetRenderPass().GetFramebuffer().CalcSamplesCount());
    auto new_pipeline = m_pipelineBuilder.Make(GetContext().GetGpuConnection().GetDevice(),
                                               GetRenderPass().GetHandle(), m_subpassIndex,
                                               m_pipelineLayout);
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipeline, nullptr);
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "VkPipeline({}) has been rebuilt - {}",
                     static_cast<void *>(m_pipeline), static_cast<void *>(new_pipeline));
    m_pipeline = new_pipeline;
    m_invalidPipeline = false;
    m_invalidPipeline.notify_one();
    SetDirtyCommands();
  }
}

void Pipeline::SetInvalid()
{
  SetDirtyCommands();
  m_descriptorsLayout.SetInvalid();
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
  m_invalidPipelineLayout = true;
}

bool Pipeline::WaitForPipelineIsValid() const noexcept
{
  bool result = false;
  for (int i = 0; i < 1000; ++i)
  {
    if (m_invalidPipeline == false)
      return true;
  }
  return false;
}

void Pipeline::BindToCommandBuffer(details::CommandBuffer & commands,
                                               VkPipelineBindPoint bindPoint)
{
  assert(!!m_pipeline);
  commands.PushCommand(vkCmdBindPipeline, bindPoint, m_pipeline);
}

void Pipeline::OnDescriptorChanged(UpdateDescriptorTask task) noexcept
{
  m_execDescriptorBuffer.UpdateDescriptor(task);
  m_writeDescriptorBuffer.UpdateDescriptor(task);
  SetDirtyCommands();
}

void Pipeline::SetDirtyCommands() noexcept
{
  m_dirtyCommands = true;
}

const SubpassLayout & Pipeline::GetLayout() const & noexcept
{
  return m_layout;
}

SubpassLayout & Pipeline::GetLayout() & noexcept
{
  return m_layout;
}

const DescriptorBufferLayout & Pipeline::GetDescriptorsLayout() const & noexcept
{
  return m_descriptorsLayout;
}

DescriptorBufferLayout & Pipeline::GetDescriptorsLayout() & noexcept
{
  return m_descriptorsLayout;
}

DescriptorBuffer & Pipeline::GetDescriptorBuffer() & noexcept
{
  return m_writeDescriptorBuffer;
}

} // namespace RHI::vulkan
