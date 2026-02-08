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
  std::lock_guard lk{m_write_lock};
}

bool Subpass::BeginPass()
{
  GetRenderPass().WaitForRenderPassIsValid(); // wait for render pass is valid
  assert(GetRenderPass().GetHandle());
  if (!m_pipeline.WaitForPipelineIsValid()) // wait while Pipeline has been invalidated
    return false;
  assert(m_pipeline.GetPipelineHandle());

  m_write_lock.lock();
  m_cachedRenderPass = GetRenderPass().GetHandle();
  m_writeBuffer.Reset();
  m_writeBuffer.BeginWriting(m_cachedRenderPass, m_pipeline.GetSubpassIndex());
  m_pipeline.BindToCommandBuffer(m_writeBuffer.GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS);
  m_writeDescriptorBuffer.BindToCommandBuffer(m_writeBuffer.GetHandle(),
                                              m_pipeline.GetPipelineLayoutHandle(),
                                              VK_PIPELINE_BIND_POINT_GRAPHICS);
  return true;
}

void Subpass::EndPass()
{
  m_writeBuffer.EndWriting();
  m_cachedRenderPass = VK_NULL_HANDLE;
  m_dirtyCommands = false;
  m_write_lock.unlock();
  m_shouldSwapBuffer = true;
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
  return m_enabled && !m_execBuffer.IsEmpty();
}

bool Subpass::ShouldBeInvalidated() const noexcept
{
  return m_dirtyCommands;
}

void Subpass::DrawVertices(std::uint32_t vertexCount, std::uint32_t instanceCount,
                           std::uint32_t firstVertex, std::uint32_t firstInstance)
{
  m_writeBuffer.PushCommand(vkCmdDraw, vertexCount, instanceCount, firstVertex, firstInstance);
}

void Subpass::DrawIndexedVertices(std::uint32_t indexCount, std::uint32_t instanceCount,
                                  std::uint32_t firstIndex, int32_t vertexOffset,
                                  std::uint32_t firstInstance)
{
  m_writeBuffer.PushCommand(vkCmdDrawIndexed, indexCount, instanceCount, firstIndex, vertexOffset,
                            firstInstance);
}

void Subpass::SetViewport(float width, float height)
{
  VkViewport vp{0.0f, 0.0f, width, height, 0.0f, 1.0f};
  m_writeBuffer.PushCommand(vkCmdSetViewport, 0, 1, &vp);
}

void Subpass::SetScissor(int32_t x, int32_t y, std::uint32_t width, std::uint32_t height)
{
  VkRect2D scissor{};
  scissor.extent = {width, height};
  scissor.offset = {x, y};
  m_writeBuffer.PushCommand(vkCmdSetScissor, 0, 1, &scissor);
}

void Subpass::BindVertexBuffer(std::uint32_t binding, const IBufferGPU & buffer,
                               std::uint32_t offset)
{
  VkDeviceSize vkOffset = offset;
  auto && vkBuffer = utils::CastInterfaceClass2Internal<BufferGPU>(buffer);
  VkBuffer buf = vkBuffer.GetHandle();
  m_writeBuffer.PushCommand(vkCmdBindVertexBuffers, 0, 1, &buf, &vkOffset);
}

void Subpass::BindIndexBuffer(const IBufferGPU & buffer, IndexType type, std::uint32_t offset)
{
  auto && vkBuffer = utils::CastInterfaceClass2Internal<BufferGPU>(buffer);
  m_writeBuffer.PushCommand(vkCmdBindIndexBuffer, vkBuffer.GetHandle(), VkDeviceSize{offset},
                            utils::CastInterfaceEnum2Vulkan<VkIndexType>(type));
}

void Subpass::PushConstant(const void * data, size_t size)
{
  m_writeBuffer.PushCommand(vkCmdPushConstants, m_pipeline.GetPipelineLayoutHandle(),
                            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                            static_cast<uint32_t>(size), data);
}

bool Subpass::ShouldSwapCommandBuffers() const noexcept
{
  return m_shouldSwapBuffer;
}

void Subpass::SwapCommandBuffers() noexcept
{
  {
    std::lock_guard lk{m_write_lock};
    std::swap(m_execBuffer, m_writeBuffer);
    std::swap(m_execDescriptorBuffer, m_writeDescriptorBuffer);
  }
  m_shouldSwapBuffer = false;
}

void Subpass::SetDirtyCacheCommands() noexcept
{
  m_dirtyCommands = true;
}

void Subpass::TransitLayoutForUsedImages(details::CommandBuffer & commandBuffer)
{
  m_pipeline.TransitLayoutForUsedImages(commandBuffer);
}

const details::CommandBuffer & Subpass::GetCommandBufferForExecution() const & noexcept
{
  return m_execBuffer;
}

details::CommandBuffer & Subpass::GetCommandBufferForWriting() & noexcept
{
  return m_writeBuffer;
}

const SubpassLayout & Subpass::GetLayout() const & noexcept
{
  return m_layout;
}

SubpassLayout & Subpass::GetLayout() & noexcept
{
  return m_layout;
}

void Subpass::SetInvalid()
{
  SetDirtyCacheCommands();
  m_pipeline.SetInvalid();
}

void Subpass::Invalidate()
{
  m_pipeline.Invalidate();
  m_execDescriptorBuffer.Invalidate();
  m_writeDescriptorBuffer.Invalidate();
}

} // namespace RHI::vulkan
