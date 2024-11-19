#include "Subpass.hpp"

#include "../CommandsExecution/CommandBuffer.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"

namespace RHI::vulkan
{
Subpass::Subpass(const Context & ctx, const RenderPass & ownerPass, uint32_t subpassIndex,
                 uint32_t familyIndex)
  : m_context(ctx)
  , m_ownerPass(ownerPass)
  , m_executableBuffer(
      new details::CommandBuffer(ctx, familyIndex, VK_COMMAND_BUFFER_LEVEL_SECONDARY))
  , m_writingBuffer(new details::CommandBuffer(ctx, familyIndex, VK_COMMAND_BUFFER_LEVEL_SECONDARY))
  , m_pipeline(new Pipeline(ctx, ownerPass, subpassIndex))
{
}

Subpass::~Subpass()
{
  std::lock_guard lk{m_write_lock};
  m_executableBuffer.reset();
}

void Subpass::BeginPass()
{
  m_ownerPass.WaitForReadyToRendering();
  assert(m_ownerPass.GetHandle());
  m_cachedRenderPass = m_ownerPass.GetHandle();
  m_pipeline->Invalidate();
  GraphicsCommands::BindCommandBuffer(m_writingBuffer->GetHandle());
  m_writingBuffer->Reset();
  m_writingBuffer->BeginWriting(m_cachedRenderPass, m_pipeline->GetSubpass());
  m_pipeline->Bind(m_writingBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS);
}

void Subpass::EndPass()
{
  m_writingBuffer->EndWriting();
  m_cachedRenderPass = VK_NULL_HANDLE;
  {
    std::lock_guard lk{m_write_lock};
    std::swap(m_executableBuffer, m_writingBuffer);
  }
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
