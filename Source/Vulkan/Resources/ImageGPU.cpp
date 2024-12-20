#include "ImageGPU.hpp"

#include <vk_mem_alloc.h>

#include "../Utils/CastHelper.hpp"
#include "../Utils/ImageUtils.hpp"
#include "../VulkanContext.hpp"
#include "Transferer.hpp"

namespace RHI::vulkan
{
namespace details
{
std::tuple<VkImage, VmaAllocation, VmaAllocationInfo> CreateVMAImage(
  VmaAllocator allocator, VmaAllocationCreateFlags flags, const ImageCreateArguments & args,
  VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO)
{
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = utils::CastInterfaceEnum2Vulkan<VkImageType>(args.type);
  imageInfo.extent.width = args.extent.width;
  imageInfo.extent.height = args.extent.height;
  imageInfo.extent.depth = args.extent.depth;
  imageInfo.mipLevels = args.mipLevels;
  imageInfo.arrayLayers = 1;
  imageInfo.format = utils::CastInterfaceEnum2Vulkan<VkFormat>(args.format);
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = utils::CastInterfaceEnum2Vulkan<VkImageUsageFlags>(args.usage);
  imageInfo.samples = utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(args.samples);
  imageInfo.sharingMode = args.shared ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = memory_usage;
  allocCreateInfo.flags = flags;
  allocCreateInfo.priority = 1.0f;
  VkImage image;
  VmaAllocation allocation;
  VmaAllocationInfo allocInfo;

  if (auto res =
        vmaCreateImage(allocator, &imageInfo, &allocCreateInfo, &image, &allocation, &allocInfo);
      res != VK_SUCCESS)
    throw std::invalid_argument("Failed to create VkImage");

  return {image, allocation, allocInfo};
}


} // namespace details

ImageGPU::ImageGPU(const Context & ctx, const details::BuffersAllocator & allocator,
                   Transferer & transferer, const ImageCreateArguments & args)
  : BufferBase(allocator, &transferer)
  , m_context(ctx)
{
  VmaAllocationCreateFlags allocation_flags = 0;
  allocation_flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

  auto allocatorHandle = reinterpret_cast<VmaAllocator>(m_allocator.GetHandle());
  auto [image, allocation, alloc_info] =
    details::CreateVMAImage(allocatorHandle, allocation_flags, args);
  m_image = image;
  m_memBlock = allocation;
  m_allocInfo = reinterpret_cast<BufferBase::AllocInfoRawMemory &>(alloc_info);
  m_args = args;
  m_flags = allocation_flags;
  m_internalFormat = utils::CastInterfaceEnum2Vulkan<VkFormat>(args.format);
}

ImageGPU::~ImageGPU()
{
  auto allocatorHandle = reinterpret_cast<VmaAllocator>(m_allocator.GetHandle());
  if (!!m_image)
    vmaDestroyImage(allocatorHandle, m_image, reinterpret_cast<VmaAllocation>(m_memBlock));
}

void ImageGPU::UploadImage(const uint8_t * data, const CopyImageArguments & args)
{
  if (!m_transferer)
    throw std::runtime_error("Image has no transferer. Async upload is impossible");
  BufferGPU stagingBuffer(utils::GetSizeForImageRegion(args.dst, m_internalFormat),
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT, m_allocator);
  if (auto && mapped_ptr = stagingBuffer.Map())
  {
    utils::CopyImageRegion_Host2GPU(data, args.src, args.format,
                                    reinterpret_cast<uint8_t *>(mapped_ptr.get()), args.dst,
                                    m_internalFormat);
    stagingBuffer.Flush();
    m_transferer->UploadImage(this, std::move(stagingBuffer));
  }
  else
  {
    throw std::runtime_error("Failed to fill staging buffer");
  }
}

VkImage ImageGPU::GetHandle() const noexcept
{
  return static_cast<VkImage>(m_image);
}

void ImageGPU::SetImageLayout(details::CommandBuffer & commandBuffer,
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
  barrier.subresourceRange.levelCount = m_args.mipLevels;
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

ImageType ImageGPU::GetImageType() const noexcept
{
  return m_args.type;
}

ImageFormat ImageGPU::GetImageFormat() const noexcept
{
  return m_args.format;
}

void ImageGPU::Invalidate() noexcept
{
}

} // namespace RHI::vulkan
