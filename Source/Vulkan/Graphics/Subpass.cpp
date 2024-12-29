#include "Subpass.hpp"

#include "../CommandsExecution/CommandBuffer.hpp"
#include "../Resources/BufferGPU.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
#include "RenderPass.hpp"

namespace RHI::vulkan
{
Subpass::Subpass(const Context & ctx, const RenderPass & ownerPass, uint32_t subpassIndex,
                 uint32_t familyIndex)
  : m_context(ctx)
  , m_ownerPass(ownerPass)
  , m_executableBuffer(ctx, familyIndex, VK_COMMAND_BUFFER_LEVEL_SECONDARY)
  , m_writingBuffer(ctx, familyIndex, VK_COMMAND_BUFFER_LEVEL_SECONDARY)
  , m_pipeline(ctx, *this, ownerPass, subpassIndex)
{
}

Subpass::~Subpass()
{
  std::lock_guard lk{m_write_lock};
}

void Subpass::BeginPass()
{
  m_ownerPass.WaitForReadyToRendering();
  assert(m_ownerPass.GetHandle());
  m_cachedRenderPass = m_ownerPass.GetHandle();
  m_pipeline.Invalidate();
  m_writingBuffer.Reset();
  m_writingBuffer.BeginWriting(m_cachedRenderPass, m_pipeline.GetSubpass());
  m_pipeline.BindToCommandBuffer(m_writingBuffer.GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS);
}

void Subpass::EndPass()
{
  m_writingBuffer.EndWriting();
  m_cachedRenderPass = VK_NULL_HANDLE;
  {
    std::lock_guard lk{m_write_lock};
    std::swap(m_executableBuffer, m_writingBuffer);
  }
  m_shouldBeInvalidated = false;
}

IPipeline & Subpass::GetConfiguration() & noexcept
{
  return m_pipeline;
}

void Subpass::SetEnabled(bool enabled) noexcept
{
  m_enabled.store(enabled);
}

bool Subpass::IsEnabled() const noexcept
{
  return m_enabled && !m_executableBuffer.IsEmpty();
}

bool Subpass::ShouldBeInvalidated() const noexcept
{
  return m_shouldBeInvalidated;
}

void Subpass::DrawVertices(std::uint32_t vertexCount, std::uint32_t instanceCount,
                           std::uint32_t firstVertex, std::uint32_t firstInstance)
{
  m_writingBuffer.PushCommand(vkCmdDraw, vertexCount, instanceCount, firstVertex, firstInstance);
}

void Subpass::DrawIndexedVertices(std::uint32_t indexCount, std::uint32_t instanceCount,
                                  std::uint32_t firstIndex, int32_t vertexOffset,
                                  std::uint32_t firstInstance)
{
  m_writingBuffer.PushCommand(vkCmdDrawIndexed, indexCount, instanceCount, firstIndex, vertexOffset,
                              firstInstance);
}

void Subpass::SetViewport(float width, float height)
{
  VkViewport vp{0.0f, 0.0f, width, height, 0.0f, 1.0f};
  m_writingBuffer.PushCommand(vkCmdSetViewport, 0, 1, &vp);
}

void Subpass::SetScissor(int32_t x, int32_t y, std::uint32_t width, std::uint32_t height)
{
  VkRect2D scissor{};
  scissor.extent = {width, height};
  scissor.offset = {x, y};
  m_writingBuffer.PushCommand(vkCmdSetScissor, 0, 1, &scissor);
}

void Subpass::BindVertexBuffer(std::uint32_t binding, const IBufferGPU & buffer,
                               std::uint32_t offset)
{
  VkDeviceSize vkOffset = offset;
  auto && vkBuffer = utils::CastInterfaceClass2Internal<BufferGPU>(buffer);
  VkBuffer buf = vkBuffer.GetHandle();
  m_writingBuffer.PushCommand(vkCmdBindVertexBuffers, 0, 1, &buf, &vkOffset);
}

void Subpass::BindIndexBuffer(const IBufferGPU & buffer, IndexType type, std::uint32_t offset)
{
  auto && vkBuffer = utils::CastInterfaceClass2Internal<BufferGPU>(buffer);
  m_writingBuffer.PushCommand(vkCmdBindIndexBuffer, vkBuffer.GetHandle(), VkDeviceSize{offset},
                              utils::CastInterfaceEnum2Vulkan<VkIndexType>(type));
}


/* void Subpass::Invalidate(){
}*/

void Subpass::LockWriting(bool lock) const noexcept
{
  if (lock)
    m_write_lock.lock();
  else
    m_write_lock.unlock();
}

void Subpass::SetDirtyCacheCommands() noexcept
{
  m_shouldBeInvalidated = true;
}

} // namespace RHI::vulkan
