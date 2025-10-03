#include "CommandBuffer.hpp"

#include <VulkanContext.hpp>

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


VkCommandBuffer CreateCommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBufferLevel level)
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

CommandBuffer::CommandBuffer(Context & ctx, uint32_t queue_family, VkCommandBufferLevel level)
  : OwnedBy<Context>(ctx)
  , m_level(level)
  , m_pool(::CreateCommandPool(ctx.GetGpuConnection().GetDevice(), queue_family))
  , m_buffer(::CreateCommandBuffer(ctx.GetGpuConnection().GetDevice(), m_pool, level))
{
}

CommandBuffer::~CommandBuffer()
{
  if (!!m_buffer)
  {
    const VkCommandBuffer buf = m_buffer;
    vkFreeCommandBuffers(GetContext().GetGpuConnection().GetDevice(), m_pool, 1, &buf);
  }
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pool, nullptr);
}

CommandBuffer::CommandBuffer(CommandBuffer && rhs) noexcept
  : OwnedBy<Context>(std::move(rhs))
{
  std::swap(rhs.m_pool, m_pool);
  std::swap(rhs.m_buffer, m_buffer);
  std::swap(rhs.m_commandsCount, m_commandsCount);
  std::swap(rhs.m_level, m_level);
}

CommandBuffer & CommandBuffer::operator=(CommandBuffer && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<Context>::operator=(std::move(rhs));
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
}

void CommandBuffer::EndWriting() const
{
  if (vkEndCommandBuffer(m_buffer) != VK_SUCCESS)
    throw std::runtime_error("failed to record command buffer!");
}

void CommandBuffer::Reset()
{
  vkResetCommandBuffer(m_buffer, 0);
  m_commandsCount = 0;
}

void CommandBuffer::AddCommands(const std::vector<VkCommandBuffer> & buffers)
{
  assert(m_level == VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  vkCmdExecuteCommands(m_buffer, static_cast<uint32_t>(buffers.size()), buffers.data());
}

} // namespace RHI::vulkan::details
