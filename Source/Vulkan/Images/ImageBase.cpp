#include "ImageBase.hpp"

#include "../CommandsExecution/CommandBuffer.hpp"
#include "../Resources/BufferGPU.hpp"
#include "../Resources/Transferer.hpp"
#include "../Utils/CastHelper.hpp"
#include "ImageFormatsConversation.hpp"
#include "ImageTraits.hpp"

namespace RHI::vulkan
{
ImageBase::ImageBase(const Context & ctx, Transferer * transferer,
                     const ImageDescription & description)
  : m_context(ctx)
  , m_transferer(transferer)
  , m_description(description)
{
}


ImageBase::ImageBase(ImageBase && rhs) noexcept
  : m_context(rhs.m_context)
{
  std::swap(m_transferer, rhs.m_transferer);
  std::swap(m_image, rhs.m_image);
  std::swap(m_description, rhs.m_description);
}

ImageBase & ImageBase::operator=(ImageBase && rhs) noexcept
{
  if (this != &rhs && &m_context == &rhs.m_context)
  {
    std::swap(m_transferer, rhs.m_transferer);
    std::swap(m_image, rhs.m_image);
    std::swap(m_description, rhs.m_description);
  }
  return *this;
}

void ImageBase::UploadImage(const uint8_t * data, const CopyImageArguments & args)
{
  if (!m_transferer)
    throw std::runtime_error("Image has no transferer. Async upload is impossible");
  BufferGPU stagingBuffer(m_context, nullptr, Size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  if (auto && mapped_ptr = stagingBuffer.Map())
  {
    utils::CopyImageFromHost(data, args.src.extent, args.src, args.hostFormat,
                             reinterpret_cast<uint8_t *>(mapped_ptr.get()), args.dst.extent,
                             args.dst, GetVulkanFormat());
    mapped_ptr.reset();
    stagingBuffer.Flush();
    m_transferer->UploadImage(this, std::move(stagingBuffer));
  }
  else
  {
    throw std::runtime_error("Failed to fill staging buffer");
  }
}

void ImageBase::DownloadImage(void * srcPixelData, const CopyImageArguments & args)
{
}

void ImageBase::SetImageLayout(details::CommandBuffer & commandBuffer,
                               VkImageLayout newLayout) noexcept
{
  VkImageMemoryBarrier barrier{};
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
  barrier.srcAccessMask = 0; // TODO
  barrier.dstAccessMask = 0; // TODO

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (m_layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (m_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
           newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else
  {
  }

  commandBuffer.PushCommand(vkCmdPipelineBarrier, sourceStage, destinationStage, 0, 0, nullptr, 0,
                            nullptr, 1, &barrier);
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
  return utils::GetSizeOfImage(GetVulkanExtent(), GetVulkanFormat());
}

ImageDescription ImageBase::GetDescription() const noexcept
{
  return m_description;
}

} // namespace RHI::vulkan
