#include "Subpass.hpp"

#include "../CommandsExecution/CommandBuffer.hpp"
#include "../Resources/BufferGPU.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
#include "RenderPass.hpp"

namespace RHI::vulkan
{
Subpass::Subpass(Context & ctx, RenderPass & ownerPass, uint32_t subpassIndex, uint32_t familyIndex)
  : OwnedBy<Context>(ctx)
  , OwnedBy<RenderPass>(ownerPass)
  , m_executableBuffer(ctx, familyIndex, VK_COMMAND_BUFFER_LEVEL_SECONDARY)
  , m_writingBuffer(ctx, familyIndex, VK_COMMAND_BUFFER_LEVEL_SECONDARY)
  , m_pipeline(ctx, *this, subpassIndex)
{
}

Subpass::~Subpass()
{
  std::lock_guard lk{m_write_lock};
}

void Subpass::BeginPass()
{
  GetRenderPass().WaitForReadyToRendering();
  assert(GetRenderPass().GetHandle());
  m_cachedRenderPass = GetRenderPass().GetHandle();
  // wait while Pipeline has been invalidated
  std::atomic_wait(&m_invalidPipeline, true);
  m_writingBuffer.Reset();
  m_writingBuffer.BeginWriting(m_cachedRenderPass, m_pipeline.GetSubpassIndex());
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
  m_dirtyCommands = false;
}

ISubpassConfiguration & Subpass::GetConfiguration() & noexcept
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
  return m_dirtyCommands;
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

void Subpass::PushConstant(const void * data, size_t size)
{
  m_writingBuffer.PushCommand(vkCmdPushConstants, m_pipeline.GetPipelineLayoutHandle(),
                              VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                              static_cast<uint32_t>(size), data);
}

void Subpass::LockWriting(bool lock) const noexcept
{
  if (lock)
    m_write_lock.lock();
  else
    m_write_lock.unlock();
}

void Subpass::SetDirtyCacheCommands() noexcept
{
  m_dirtyCommands = true;
}

void Subpass::TransitLayoutForUsedImages(details::CommandBuffer & commandBuffer)
{
  m_pipeline.TransitLayoutForUsedImages(commandBuffer);
}

const SubpassLayout & Subpass::GetLayout() const & noexcept
{
  return m_layout;
}

SubpassLayout & Subpass::GetLayout() & noexcept
{
  return m_layout;
}

void Subpass::Invalidate()
{
  if (m_invalidPipeline)
  {
    m_pipeline.Invalidate();
    SetDirtyCacheCommands();
    m_invalidPipeline = false;
    m_invalidPipeline.notify_one();
  }
}

} // namespace RHI::vulkan
