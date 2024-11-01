#include "GraphicsExecutor.hpp"

namespace
{
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


vk::CommandBuffer CreateCommandBuffer(vk::Device device, vk::CommandPool pool,
                                      RHI::CommandBufferType type)
{
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = pool;
  allocInfo.level = type == RHI::CommandBufferType::Executable ? VK_COMMAND_BUFFER_LEVEL_PRIMARY
                                                               : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate command buffers!");

  return vk::CommandBuffer{commandBuffer};
}
} // namespace

namespace RHI::vulkan
{

GraphicsExecutor::GraphicsExecutor(const Context & context, GraphicsExecutor * owner)
  : m_context(context)
  , m_owner(owner)
{
  m_buffersType = m_owner == nullptr ? RHI::CommandBufferType::Executable
                                     : RHI::CommandBufferType::ThreadLocal;
  if (m_owner == nullptr)
  {
    auto [familyIndex, queue] = context.GetQueue(RHI::vulkan::QueueType::Graphics);
    m_pool = utils::CreateCommandPool(context.GetDevice(), familyIndex);
  }
  else
  {
    m_pool = m_owner->m_pool;
  }

  m_writing_commands_buffer = ::CreateCommandBuffer(m_context.GetDevice(), m_pool, m_buffersType);
  m_child_commands_buffer = ::CreateCommandBuffer(m_context.GetDevice(), m_pool, m_buffersType);
  m_commited_commands_buffer = ::CreateCommandBuffer(m_context.GetDevice(), m_pool, m_buffersType);
}

GraphicsExecutor::~GraphicsExecutor()
{
  std::array<VkCommandBuffer, 3> buffersToFree = {m_commited_commands_buffer,
                                                  m_child_commands_buffer,
                                                  m_writing_commands_buffer};
  vkFreeCommandBuffers(m_context.GetDevice(), m_pool, buffersToFree.size(), buffersToFree.data());
}

void GraphicsExecutor::BeginExecute(const IFramebuffer & framebuffer)
{
  CancelExecute();
  VkCommandBufferInheritanceInfo inheritanceInfo{};
  if (m_buffersType == CommandBufferType::ThreadLocal)
  {
    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.renderPass = reinterpret_cast<VkRenderPass>(framebuffer.GetRenderPass());
    //inheritanceInfo.framebuffer = reinterpret_cast<VkFramebuffer>(framebuffer.GetHandle());
    inheritanceInfo.subpass = pipeline.GetSubpass();
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = m_buffersType == CommandBufferType::ThreadLocal
                    ? VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT |
                        VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT
                    : 0; // Optional
  beginInfo.pInheritanceInfo = m_buffersType == CommandBufferType::ThreadLocal ? &inheritanceInfo
                                                                               : nullptr;
  if (vkBeginCommandBuffer(m_writing_commands_buffer, &beginInfo) != VK_SUCCESS)
    throw std::runtime_error("failed to begin recording command buffer!");
}

void GraphicsExecutor::EndExecute()
{
  if (vkEndCommandBuffer(m_writing_commands_buffer) != VK_SUCCESS)
    throw std::runtime_error("failed to record command buffer!");
  {
    std::lock_guard lk{m};
    std::swap(m_writing_commands_buffer, m_commited_commands_buffer);
  }
  if (m_owner)
    m_owner->SetChildCommandsBufferDirty();
}

void GraphicsExecutor::CancelExecute()
{
  if (vkEndCommandBuffer(m_writing_commands_buffer) != VK_SUCCESS)
    throw std::runtime_error("failed to record command buffer!");
  vkResetCommandBuffer(m_writing_commands_buffer, 0);
}

IGraphicsCommandsExecutor & GraphicsExecutor::CreateThreadLocalExecutor() &
{
  auto && child = m_child_executors.emplace_back(m_context, this);
  SetChildCommandsBufferDirty();
  return child;
}


} // namespace RHI::vulkan
