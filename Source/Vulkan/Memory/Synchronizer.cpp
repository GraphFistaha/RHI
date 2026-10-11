#include "Synchronizer.hpp"

#include "LegacySynchronization.hpp"

#include <CommandsExecution/CommandBuffer.hpp>
#include <VulkanContext.hpp>

namespace
{
bool IsFramebufferSpaceStage(VkPipelineStageFlags stage) noexcept
{
  const VkPipelineStageFlags framebufferStages =
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  return static_cast<bool>(stage & framebufferStages);
}
} // namespace

namespace RHI::vulkan::details
{

Synchronizer::Synchronizer(Context & ctx, VkImage image)
  : OwnedBy<Context>(ctx)
  , m_image(image)
{
  std::lock_guard lk{m_syncMutex};
  m_prevBarrier = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED};
}

Synchronizer::Synchronizer(Context & ctx, VkBuffer buffer)
  : OwnedBy<Context>(ctx)
  , m_buffer(buffer)
{
  std::lock_guard lk{m_syncMutex};
  m_prevBarrier = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED};
}

Synchronizer::Synchronizer(Synchronizer && rhs) noexcept
  : OwnedBy<Context>(std::move(rhs))
{
  std::lock_guard lk1(m_syncMutex);
  std::lock_guard lk2(rhs.m_syncMutex);
  std::swap(m_image, rhs.m_image);
  std::swap(m_buffer, rhs.m_buffer);
  std::swap(m_prevBarrier, rhs.m_prevBarrier);
}

Synchronizer & Synchronizer::operator=(Synchronizer && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<Context>::operator=(std::move(rhs));
    std::lock_guard lk1(m_syncMutex);
    std::lock_guard lk2(rhs.m_syncMutex);
    std::swap(m_image, rhs.m_image);
    std::swap(m_buffer, rhs.m_buffer);
    std::swap(m_prevBarrier, rhs.m_prevBarrier);
  }
  return *this;
}

Synchronizer::~Synchronizer()
{
}

void Synchronizer::ResetSynchronization()
{
  std::lock_guard lk{m_syncMutex};
  m_prevBarrier = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                   // don't reset layout for images
                   m_buffer ? VK_IMAGE_LAYOUT_UNDEFINED : m_prevBarrier.requiredLayout};
}

void Synchronizer::RequireSynchronize(VkPipelineStageFlags2 currentStage,
                                      VkAccessFlagBits2 requiredAccess,
                                      details::CommandBuffer & commands,
                                      VkImageLayout requiredLayout /* = VK_IMAGE_LAYOUT_UNDEFINED*/)
{
  assert(!m_image || requiredLayout != VK_IMAGE_LAYOUT_UNDEFINED);
  ResourceState barrierInfo{};
  barrierInfo.currentStage = currentStage;
  barrierInfo.requiredAccess = requiredAccess;
  barrierInfo.requiredLayout = requiredLayout;
  VkDependencyFlags dependencyFlags = 0;
  if (IsFramebufferSpaceStage(m_prevBarrier.currentStage))
    dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
  std::lock_guard lk{m_syncMutex};
  if (GetContext().GetGpuConnection().CheckExtension("VK_KHR_synchronization2") &&
      GetContext().GetGpuConnection().GetVulkanVersion() >= VK_API_VERSION_1_3)
  {
    VkDependencyInfo info{};
    VkImageMemoryBarrier2 imageBarrier{};
    VkBufferMemoryBarrier2 bufferBarrier{};
    info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    info.dependencyFlags = dependencyFlags;
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
  }
  else
  {
    VkImageMemoryBarrier imageBarrier{};
    VkBufferMemoryBarrier bufferBarrier{};
    if (m_image)
    {
      imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      imageBarrier.pNext = nullptr;
      imageBarrier.image = m_image;
      imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageBarrier.subresourceRange.baseMipLevel = 0;
      imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
      imageBarrier.subresourceRange.baseArrayLayer = 0;
      imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
      imageBarrier.dstAccessMask = ConvertAccessMaskFromVk2ToVk1(requiredAccess);
      imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imageBarrier.srcAccessMask = ConvertAccessMaskFromVk2ToVk1(m_prevBarrier.requiredAccess);
      imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imageBarrier.newLayout = requiredLayout;
      imageBarrier.oldLayout = m_prevBarrier.requiredLayout;
    }
    else if (m_buffer)
    {
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.pNext = nullptr;
      bufferBarrier.buffer = m_buffer;
      bufferBarrier.offset = 0;
      bufferBarrier.size = std::numeric_limits<VkDeviceSize>::max();
      bufferBarrier.dstAccessMask = ConvertAccessMaskFromVk2ToVk1(requiredAccess);
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.srcAccessMask = ConvertAccessMaskFromVk2ToVk1(m_prevBarrier.requiredAccess);
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }
    else
    {
      throw std::runtime_error("Unknown object for barrier");
    }
    const auto sourceStages = ConvertPipelineStageFromVk2ToVk1(m_prevBarrier.currentStage);
    const auto destinationStages = ConvertPipelineStageFromVk2ToVk1(currentStage);
    // Empty synchronization2 scopes use legacy execution endpoints without accesses.
    if ((!sourceStages && m_prevBarrier.requiredAccess) || (!destinationStages && requiredAccess))
      throw std::invalid_argument("An access scope requires a stage");
    commands.PushCommand(vkCmdPipelineBarrier,
                         sourceStages ? sourceStages : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         destinationStages ? destinationStages : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         dependencyFlags, 0,
                         nullptr, m_buffer ? 1 : 0, &bufferBarrier, m_image ? 1 : 0, &imageBarrier);
  }
  m_prevBarrier = barrierInfo;
}

const ResourceState & Synchronizer::GetState() const & noexcept
{
  std::lock_guard lk{m_syncMutex};
  return m_prevBarrier;
}

void Synchronizer::ExternalSynchronization(const ResourceState & barrier) noexcept
{
  std::lock_guard lk{m_syncMutex};
  m_prevBarrier = barrier;
}

} // namespace RHI::vulkan::details
