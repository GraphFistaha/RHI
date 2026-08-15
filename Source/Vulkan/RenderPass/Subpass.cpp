#include "Subpass.hpp"

#include <CommandsExecution/CommandBuffer.hpp>
#include <Memory/BufferGPU.hpp>
#include <RenderPass/RenderPass.hpp>
#include <RenderPass/SubpassConfiguration.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{
Subpass::Subpass(Context & ctx, SubpassConfiguration & owner, uint32_t subpassIndex,
                 uint32_t familyIndex)
  : OwnedBy<Context>(ctx)
  , OwnedBy<SubpassConfiguration>(owner)
  , m_execBuffer(ctx, familyIndex, VK_COMMAND_BUFFER_LEVEL_SECONDARY)
  , m_writeBuffer(ctx, familyIndex, VK_COMMAND_BUFFER_LEVEL_SECONDARY)
  , m_descriptorsLayout(ctx, *this)
  , m_execDescriptorBuffer(ctx, m_descriptorsLayout)
  , m_writeDescriptorBuffer(ctx, m_descriptorsLayout)
{
}

Subpass::~Subpass()
{
}

void Subpass::RecordCommands(details::CommandBuffer & commands)
{
  GetPipeline().GetRenderPass().WaitForRenderPassIsValid(); // wait for render pass is valid
  if (m_dirtyCommands)
  {
    m_writeBuffer.Reset();
    m_writeBuffer.BeginWriting(GetPipeline().GetRenderPass().GetHandle(),
                               GetPipeline().GetSubpassIndex());
    GetPipeline().BindToCommandBuffer(m_writeBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
    m_writeDescriptorBuffer.BindToCommandBuffer(m_writeBuffer,
                                                GetPipeline().GetPipelineLayoutHandle(),
                                                VK_PIPELINE_BIND_POINT_GRAPHICS);
    if (m_process)
      m_process->RecordCommands(m_writeBuffer, GetPipeline());

    m_writeBuffer.EndWriting();
    std::swap(m_writeBuffer, m_execBuffer);
    m_dirtyCommands = false;
  }

  commands.AddCommands(m_execBuffer.GetHandle());
}

void Subpass::CollectResources(std::vector<ResourcePtr> & resources) const
{
  // collect descriptors and uniforms
  m_descriptorsLayout.CollectResources(resources);
  // collect resources from draw commands (vertex/index buffers)
  if (m_process)
    m_process->CollectResources(resources);
}

void Subpass::SynchroniseResources(details::CommandBuffer & commands) const
{
  // collect descriptors and uniforms
  m_descriptorsLayout.SynchroniseResources(commands);
  // collect resources from draw commands (vertex/index buffers)
  if (m_process)
    m_process->SynchroniseResources(commands);
}

void Subpass::SetDirtyCommands() noexcept
{
  m_dirtyCommands = true;
}

const SubpassLayout & Subpass::GetLayout() const & noexcept
{
  return m_layout;
}

SubpassLayout & Subpass::GetLayout() & noexcept
{
  return m_layout;
}

const DescriptorBufferLayout & Subpass::GetDescriptorsLayout() const & noexcept
{
  return m_descriptorsLayout;
}

DescriptorBufferLayout & Subpass::GetDescriptorsLayout() & noexcept
{
  return m_descriptorsLayout;
}

DescriptorBuffer & Subpass::GetDescriptorBuffer() & noexcept
{
  return m_writeDescriptorBuffer;
}

void Subpass::SetRenderProcess(PipelineProcessPtr process)
{
  m_process = FastDynamicCast<PipelineProcess>(process);
  if (m_process)
    m_process->CommitProcess();
  SetDirtyCommands();
}

void Subpass::SetInvalid()
{
  SetDirtyCommands();
  m_descriptorsLayout.SetInvalid();
  GetPipeline().SetInvalid();
}

void Subpass::Invalidate()
{
  m_descriptorsLayout.Invalidate();
  GetPipeline().Invalidate();
  m_execDescriptorBuffer.Invalidate();
  m_writeDescriptorBuffer.Invalidate();
}

} // namespace RHI::vulkan
