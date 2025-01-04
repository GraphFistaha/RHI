#include "CommandBuffer.hpp"

#include "../VulkanContext.hpp"

namespace
{
VkCommandPool CreateCommandPool(VkDevice device, uint32_t queue_family_index)
{
  VkCommandPool commandPool = VK_NULL_HANDLE;

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queue_family_index;
  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    throw std::invalid_argument("failed to create command pool!");
  return VkCommandPool{commandPool};
}


VkCommandBuffer CreateCommandBuffer(VkDevice device, VkCommandPool pool,
                                      VkCommandBufferLevel level)
{
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = pool;
  allocInfo.level = level;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate command buffers!");

  return VkCommandBuffer{commandBuffer};
}
} // namespace

namespace RHI::vulkan::details
{

CommandBuffer::CommandBuffer(const Context & ctx, uint32_t queue_family, VkCommandBufferLevel level)
  : m_context(ctx)
  , m_level(level)
  , m_pool(::CreateCommandPool(ctx.GetDevice(), queue_family))
  , m_buffer(::CreateCommandBuffer(ctx.GetDevice(), m_pool, level))
{
}

CommandBuffer::~CommandBuffer()
{
  if (!!m_buffer)
  {
    const VkCommandBuffer buf = m_buffer;
    vkFreeCommandBuffers(m_context.GetDevice(), m_pool, 1, &buf);
  }
  m_context.GetGarbageCollector().PushVkObjectToDestroy(m_pool, nullptr);
}

CommandBuffer::CommandBuffer(CommandBuffer && rhs) noexcept
  : m_context(rhs.m_context)
{
  std::swap(rhs.m_pool, m_pool);
  std::swap(rhs.m_buffer, m_buffer);
  std::swap(rhs.m_commandsCount, m_commandsCount);
  std::swap(rhs.m_level, m_level);
}

CommandBuffer & CommandBuffer::operator=(CommandBuffer && rhs)
{
  if (this != &rhs)
  {
    if (&m_context != &rhs.m_context)
      throw std::logic_error(
        "Move command buffer from one context to command buffer from another context");
    std::swap(rhs.m_pool, m_pool);
    std::swap(rhs.m_buffer, m_buffer);
    std::swap(rhs.m_commandsCount, m_commandsCount);
    std::swap(rhs.m_level, m_level);
  }
  return *this;
}

void CommandBuffer::BeginWriting() const
{
  if (m_level != VK_COMMAND_BUFFER_LEVEL_PRIMARY)
    throw std::invalid_argument("Called writing in primary CommandBuffer, but buffer is secondary");
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0; // Optional
  beginInfo.pInheritanceInfo = nullptr;
  if (vkBeginCommandBuffer(m_buffer, &beginInfo) != VK_SUCCESS)
    throw std::runtime_error("failed to begin recording command buffer!");
}

void CommandBuffer::BeginWriting(VkRenderPass renderPass, uint32_t subpassIndex,
                                 VkFramebuffer framebuffer /*= VK_NULL_HANDLE*/) const
{
  if (m_level != VK_COMMAND_BUFFER_LEVEL_SECONDARY)
    throw std::invalid_argument("Called writing in primary CommandBuffer, but buffer is secondary");

  assert(!!renderPass);
  VkCommandBufferInheritanceInfo inheritanceInfo{};
  inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
  inheritanceInfo.renderPass = renderPass;
  inheritanceInfo.framebuffer = framebuffer;
  inheritanceInfo.subpass = subpassIndex;

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags =
    m_level == VK_COMMAND_BUFFER_LEVEL_SECONDARY
      ? /*VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | probably we need that only in Windows*/
      VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT
      : 0; // Optional
  beginInfo.pInheritanceInfo = &inheritanceInfo;

  if (vkBeginCommandBuffer(m_buffer, &beginInfo) != VK_SUCCESS)
    throw std::runtime_error("failed to begin recording command buffer!");

  //auto && vkPipeline = static_cast<const Pipeline &>(pipeline);
  //vkPipeline.Bind(m_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
}

void CommandBuffer::EndWriting() const
{
  if (vkEndCommandBuffer(m_buffer) != VK_SUCCESS)
    throw std::runtime_error("failed to record command buffer!");
}

/*
VkIndexType IndexType2VulkanEnum(RHI::IndexType type)
{
  using namespace RHI;
  switch (type)
  {
    case IndexType::UINT8:
      return VkIndexType::VK_INDEX_TYPE_UINT8_EXT;
    case IndexType::UINT16:
      return VkIndexType::VK_INDEX_TYPE_UINT16;
    case IndexType::UINT32:
      return VkIndexType::VK_INDEX_TYPE_UINT32;
    default:
      throw std::runtime_error("Failed to cast IndexType to vulkan enum");
  }
}

void CommandBuffer::DrawVertices(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                                 uint32_t firstInstance) const
{
  vkCmdDraw(m_buffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::DrawIndexedVertices(uint32_t indexCount, uint32_t instanceCount,
                                        uint32_t firstIndex, int32_t vertexOffset,
                                        uint32_t firstInstance) const
{
  vkCmdDrawIndexed(m_buffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::SetViewport(float width, float height)
{
  VkViewport viewport = {};
  viewport.height = height;
  viewport.width = width;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(m_buffer, 0, 1, &viewport);
}

void CommandBuffer::SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
  VkRect2D scissor = {};
  scissor.extent.width = width;
  scissor.extent.height = height;
  scissor.offset.x = x;
  scissor.offset.y = y;
  vkCmdSetScissor(m_buffer, 0, 1, &scissor);
}

void CommandBuffer::BindVertexBuffer(uint32_t binding, const IBufferGPU & buffer,
                                     uint32_t offset /* = 0)
{
  auto && bufferHandle = reinterpret_cast<VkBuffer>(buffer.GetHandle());
  VkDeviceSize vkOffset = offset;
  vkCmdBindVertexBuffers(m_buffer, binding, 1, &bufferHandle, &vkOffset);
}

void CommandBuffer::BindIndexBuffer(const IBufferGPU & buffer, IndexType type, uint32_t offset)
{
  auto && bufferHandle = reinterpret_cast<VkBuffer>(buffer.GetHandle());
  VkDeviceSize vkOffset = offset;
  vkCmdBindIndexBuffer(m_buffer, bufferHandle, vkOffset, IndexType2VulkanEnum(type));
}
*/

void CommandBuffer::Reset()
{
  vkResetCommandBuffer(m_buffer, 0);
}

void CommandBuffer::AddCommands(const std::vector<VkCommandBuffer> & buffers)
{
  assert(m_level == VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  vkCmdExecuteCommands(m_buffer, static_cast<uint32_t>(buffers.size()), buffers.data());
}

} // namespace RHI::vulkan::details
