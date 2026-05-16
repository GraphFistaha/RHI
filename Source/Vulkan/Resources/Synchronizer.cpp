#include "Synchronizer.hpp"

#include <CommandsExecution/CommandBuffer.hpp>

namespace RHI::vulkan::details
{

Synchronizer::Synchronizer(Context & ctx, VkImage image)
  : OwnedBy<Context>(ctx)
  , m_image(image)
{
  m_prevBarrier = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED};
}

Synchronizer::Synchronizer(Context & ctx, VkBuffer buffer)
  : OwnedBy<Context>(ctx)
  , m_buffer(buffer)
{
  m_prevBarrier = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED};
}

Synchronizer::~Synchronizer()
{
}

void Synchronizer::RequireSynchronize(VkPipelineStageFlags2 currentStage,
                                      VkAccessFlagBits2 requiredAccess,
                                      details::CommandBuffer & commands,
                                      VkImageLayout requiredLayout /* = VK_IMAGE_LAYOUT_UNDEFINED*/)
{
  BarrierInfo barrierInfo{};
  barrierInfo.currentStage = currentStage;
  barrierInfo.requiredAccess = requiredAccess;
  barrierInfo.requiredLayout = requiredLayout;

  VkDependencyInfo info{};
  VkImageMemoryBarrier2 imageBarrier{};
  VkBufferMemoryBarrier2 bufferBarrier{};
  info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  if (m_image)
  {
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageBarrier.pNext = nullptr;
    imageBarrier.image = m_image;
    imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    imageBarrier.dstStageMask = currentStage;
    imageBarrier.dstAccessMask = requiredAccess;
    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.srcStageMask = m_prevBarrier.currentStage;
    imageBarrier.srcAccessMask = m_prevBarrier.requiredAccess;
    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.newLayout = requiredLayout;
    imageBarrier.oldLayout = m_prevBarrier.requiredLayout;
    info.imageMemoryBarrierCount = 1;
    info.pImageMemoryBarriers = &imageBarrier;
  }
  else if (m_buffer)
  {
    bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    bufferBarrier.pNext = nullptr;
    bufferBarrier.buffer = m_buffer;
    bufferBarrier.offset = 0;
    bufferBarrier.size = std::numeric_limits<VkDeviceSize>::max();
    bufferBarrier.dstStageMask = currentStage;
    bufferBarrier.dstAccessMask = requiredAccess;
    bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.srcStageMask = m_prevBarrier.currentStage;
    bufferBarrier.srcAccessMask = m_prevBarrier.requiredAccess;
    bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    info.bufferMemoryBarrierCount = 1;
    info.pBufferMemoryBarriers = &bufferBarrier;
  }
  else
  {
    throw std::runtime_error("Unknown object for barrier");
  }

  commands.PushCommand(vkCmdPipelineBarrier2, &info);
  m_prevBarrier = barrierInfo;
}


VkImageLayout Synchronizer::GetLayout() const noexcept
{
  return m_prevBarrier.requiredLayout;
}

void Synchronizer::SetLayout(VkImageLayout layout) noexcept
{
  m_prevBarrier.requiredLayout = layout;
}

} // namespace RHI::vulkan::details
