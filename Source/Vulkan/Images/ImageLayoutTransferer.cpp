#include "ImageLayoutTransferer.hpp"

#include "../CommandsExecution/CommandBuffer.hpp"
#include "../Utils/CastHelper.hpp"
#include "InternalImageTraits.hpp"

namespace
{
constexpr VkAccessFlags LayoutTransfer_MakeAccessFlag(VkImageLayout layout) noexcept
{
  switch (layout)
  {
    case VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL:
      return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      return VK_ACCESS_SHADER_READ_BIT;

    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
      return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

    case VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
      return VK_ACCESS_SHADER_WRITE_BIT;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: // пишем в картинку
      return VK_ACCESS_TRANSFER_WRITE_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: // читаем из картинки
      return VK_ACCESS_TRANSFER_READ_BIT;

    case VK_IMAGE_LAYOUT_GENERAL:
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
    case VK_IMAGE_LAYOUT_UNDEFINED:
    default:
      return 0;
  }
}

constexpr VkPipelineStageFlags LayoutTransfer_MakePipelineStage(VkImageLayout layout) noexcept
{
  switch (layout)
  {
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: // shader sampler
      return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: // transfering
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      return VK_PIPELINE_STAGE_TRANSFER_BIT;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: // color attachment
      return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL: // depth/stencil attachment
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
      return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
    case VK_IMAGE_LAYOUT_UNDEFINED:
      return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    default:
      assert(false);
      return VK_PIPELINE_STAGE_NONE;
  }
}

} // namespace

namespace RHI::vulkan
{
ImageLayoutTransferer::ImageLayoutTransferer(VkImage image)
  : m_image(image)
{
}


ImageLayoutTransferer::ImageLayoutTransferer(ImageLayoutTransferer && rhs) noexcept
{
  std::swap(m_image, rhs.m_image);
  rhs.m_layout = m_layout.exchange(rhs.m_layout.load());
}

ImageLayoutTransferer & ImageLayoutTransferer::operator=(ImageLayoutTransferer && rhs) noexcept
{
  if (this != &rhs)
  {
    std::swap(m_image, rhs.m_image);
    rhs.m_layout = m_layout.exchange(rhs.m_layout.load());
  }
  return *this;
}

void ImageLayoutTransferer::TransferLayout(VkImageLayout newLayout) noexcept
{
  if (newLayout != VK_IMAGE_LAYOUT_UNDEFINED && newLayout != VK_IMAGE_LAYOUT_PREINITIALIZED)
    m_layout = newLayout;
}

void ImageLayoutTransferer::TransferLayout(details::CommandBuffer & commandBuffer,
                                           VkImageLayout newLayout) noexcept
{
  if (newLayout == VK_IMAGE_LAYOUT_UNDEFINED || newLayout == VK_IMAGE_LAYOUT_PREINITIALIZED ||
      newLayout == m_layout)
    return;
  VkImageMemoryBarrier barrier{};
  {
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_layout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = LayoutTransfer_MakeAccessFlag(m_layout);
    barrier.dstAccessMask = LayoutTransfer_MakeAccessFlag(newLayout);
  }

  VkPipelineStageFlags sourceStage = LayoutTransfer_MakePipelineStage(m_layout);
  VkPipelineStageFlags destinationStage = LayoutTransfer_MakePipelineStage(newLayout);

  m_layout = newLayout;
  commandBuffer.PushCommand(vkCmdPipelineBarrier, sourceStage, destinationStage, 0, 0, nullptr, 0,
                            nullptr, 1, &barrier);
}

} // namespace RHI::vulkan
