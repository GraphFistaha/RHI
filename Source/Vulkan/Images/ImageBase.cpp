#include "ImageBase.hpp"

#include "../CommandsExecution/CommandBuffer.hpp"
#include "../Resources/BufferGPU.hpp"
#include "../Resources/Transferer.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
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
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      return VK_PIPELINE_STAGE_TRANSFER_BIT;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
    case VK_IMAGE_LAYOUT_UNDEFINED:
      return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    default:
      return VK_PIPELINE_STAGE_NONE;
  }
}

} // namespace

namespace RHI::vulkan
{
ImageBase::ImageBase(Context & ctx, const ImageCreateArguments & description)
  : ContextualObject(ctx)
  , m_description(description)
{
}


ImageBase::ImageBase(ImageBase && rhs) noexcept
  : ContextualObject(std::move(rhs))
{
  std::swap(m_image, rhs.m_image);
  std::swap(m_description, rhs.m_description);
}

ImageBase & ImageBase::operator=(ImageBase && rhs) noexcept
{
  if (this != &rhs)
  {
    ContextualObject::operator=(std::move(rhs));
    std::swap(m_image, rhs.m_image);
    std::swap(m_description, rhs.m_description);
  }
  return *this;
}

std::future<UploadResult> ImageBase::UploadImage(const uint8_t * data,
                                                 const CopyImageArguments & args)
{
  return GetContext().GetTransferer().UploadImage(*this, data, args);
}

std::future<DownloadResult> ImageBase::DownloadImage(HostImageFormat format,
                                                     const ImageRegion & region)
{
  return GetContext().GetTransferer().DownloadImage(*this, format, region);
}

void ImageBase::TransferLayout(details::CommandBuffer & commandBuffer,
                               VkImageLayout newLayout) noexcept
{
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
    barrier.subresourceRange.levelCount = m_description.mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = LayoutTransfer_MakeAccessFlag(m_layout);
    barrier.dstAccessMask = LayoutTransfer_MakeAccessFlag(newLayout);
  }

  VkPipelineStageFlags sourceStage = LayoutTransfer_MakePipelineStage(m_layout);
  VkPipelineStageFlags destinationStage = LayoutTransfer_MakePipelineStage(newLayout);

  commandBuffer.PushCommand(vkCmdPipelineBarrier, sourceStage, destinationStage, 0, 0, nullptr, 0,
                            nullptr, 1, &barrier);
  m_layout = newLayout;
}

void ImageBase::SetImageLayoutByRenderPass(VkImageLayout newLayout) noexcept
{
  m_layout = newLayout;
}

VkImageType ImageBase::GetVulkanImageType() const noexcept
{
  return utils::CastInterfaceEnum2Vulkan<VkImageType>(m_description.type);
}

VkExtent3D ImageBase::GetVulkanExtent() const noexcept
{
  return VkExtent3D{m_description.extent[0], m_description.extent[1], m_description.extent[2]};
}

VkFormat ImageBase::GetVulkanFormat() const noexcept
{
  return utils::CastInterfaceEnum2Vulkan<VkFormat>(m_description.format);
}

VkSampleCountFlagBits ImageBase::GetVulkanSamplesCount() const noexcept
{
  return utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(m_description.samples);
}

size_t ImageBase::Size() const
{
  return RHI::utils::GetSizeOfImage(GetVulkanExtent(), GetVulkanFormat());
}

ImageCreateArguments ImageBase::GetDescription() const noexcept
{
  return m_description;
}

} // namespace RHI::vulkan
