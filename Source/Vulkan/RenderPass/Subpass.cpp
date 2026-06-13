#include "Subpass.hpp"

#include <CommandsExecution/CommandBuffer.hpp>
#include <RenderPass/RenderPass.hpp>
#include <Resources/BufferGPU.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{
Subpass::Subpass(Context & ctx, RenderPass & ownerPass, uint32_t subpassIndex, uint32_t familyIndex)
  : OwnedBy<Context>(ctx)
  , OwnedBy<RenderPass>(ownerPass)
  , m_pipeline(ctx, *this, subpassIndex)
  , m_execBuffer(ctx, familyIndex, VK_COMMAND_BUFFER_LEVEL_SECONDARY)
  , m_writeBuffer(ctx, familyIndex, VK_COMMAND_BUFFER_LEVEL_SECONDARY)
  , m_execDescriptorBuffer(ctx, m_pipeline.GetDescriptorsLayout())
  , m_writeDescriptorBuffer(ctx, m_pipeline.GetDescriptorsLayout())
{
}

Subpass::~Subpass()
{
}

ISubpassConfiguration & Subpass::GetConfiguration() & noexcept
{
  return m_pipeline;
}

void Subpass::RecordCommands(details::CommandBuffer & commands)
{
  GetRenderPass().WaitForRenderPassIsValid(); // wait for render pass is valid
  m_cachedRenderPass = GetRenderPass().GetHandle();
  assert(GetRenderPass().GetHandle());
  if (m_dirtyCommands)
  {
    m_writeBuffer.Reset();
    m_writeBuffer.BeginWriting(m_cachedRenderPass, m_pipeline.GetSubpassIndex());
    m_pipeline.BindToCommandBuffer(m_writeBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
    m_writeDescriptorBuffer.BindToCommandBuffer(m_writeBuffer, m_pipeline.GetPipelineLayoutHandle(),
                                                VK_PIPELINE_BIND_POINT_GRAPHICS);
    if (m_process)
      m_process->RecordCommands(m_writeBuffer, m_pipeline);

    m_writeBuffer.EndWriting();
    std::swap(m_writeBuffer, m_execBuffer);
    m_dirtyCommands = false;
  }

  commands.AddCommands(m_execBuffer.GetHandle());
}

void Subpass::SetDirtyCommands() noexcept
{
  m_dirtyCommands = true;
}

void Subpass::SynchroniseResources(details::CommandBuffer & commands)
{
  m_pipeline.SynchroniseResources(commands);
  if (m_process)
    m_process->SynchroniseResources(commands);
}

const SubpassLayout & Subpass::GetLayout() const & noexcept
{
  return m_layout;
}

SubpassLayout & Subpass::GetLayout() & noexcept
{
  return m_layout;
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
  m_pipeline.SetInvalid();
}

void Subpass::Invalidate()
{
  m_pipeline.Invalidate();
  m_execDescriptorBuffer.Invalidate();
  m_writeDescriptorBuffer.Invalidate();
}

} // namespace RHI::vulkan
