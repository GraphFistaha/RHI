#include "Subpass.hpp"

#include "../CommandsExecution/CommandBuffer.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"

namespace RHI::vulkan
{
Subpass::Subpass(const Context & ctx, const RenderPass & ownerPass, uint32_t subpassIndex,
                 uint32_t familyIndex)
  : m_ownerPass(ownerPass)
  , m_buffer(new details::CommandBuffer(ctx, familyIndex, VK_COMMAND_BUFFER_LEVEL_SECONDARY))
  , m_pipeline(new Pipeline(ctx, ownerPass, subpassIndex))
{
  GraphicsCommands::BindCommandBuffer(m_buffer->GetHandle());
}

Subpass::~Subpass() = default;

void Subpass::BeginPass()
{
  LockWriting(true);
  m_buffer->BeginWriting(m_ownerPass.GetHandle(), m_pipeline->GetSubpass());
  m_pipeline->Bind(m_buffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS);
}

void Subpass::EndPass()
{
  LockWriting(false);
  m_buffer->EndWriting();
}

IPipeline & Subpass::GetConfiguration() & noexcept
{
  return *m_pipeline;
}

void Subpass::Invalidate()
{
}

void Subpass::LockWriting(bool lock) const noexcept
{
  if (lock)
    m_write_lock.lock();
  else
    m_write_lock.unlock();
}

} // namespace RHI::vulkan
