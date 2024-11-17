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
  m_pipeline->Invalidate();
  m_write_lock.lock();
  m_buffer->BeginWriting(m_ownerPass.GetHandle(), m_pipeline->GetSubpass());
  m_pipeline->Bind(m_buffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS);
}

void Subpass::EndPass()
{
  m_write_lock.unlock();
  m_buffer->EndWriting();
}

IPipeline & Subpass::GetConfiguration() & noexcept
{
  return *m_pipeline;
}

void Subpass::SetEnabled(bool enabled) noexcept
{
  m_enabled.store(enabled);
}

bool Subpass::IsEnabled() const noexcept
{
  return m_enabled;
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
