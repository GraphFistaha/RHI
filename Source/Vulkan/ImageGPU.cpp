#include "ImageGPU.hpp"

#include <vk_mem_alloc.h>

namespace RHI::vulkan
{
namespace details
{
VkImageType ImageType2VulkanEnum(ImageType type)
{
  return static_cast<VkImageType>(type);
}

VkImageViewType ImageType2VulkanImageViewType(ImageType type)
{
  switch (type)
  {
    case ImageType::Image1D:
      return VK_IMAGE_VIEW_TYPE_1D;
    case ImageType::Image2D:
      return VK_IMAGE_VIEW_TYPE_2D;
    case ImageType::Image3D:
      return VK_IMAGE_VIEW_TYPE_3D;
    default:
      throw std::range_error("Invalid ImageType");
  }
}

VkFormat ImageFormat2VulkanEnum(ImageFormat format)
{
  switch (format)
  {
    case ImageFormat::RGB8:
      return VK_FORMAT_R8G8B8_SRGB;
    case ImageFormat::RGBA8:
      return VK_FORMAT_R8G8B8A8_SRGB;
    default:
      return VK_FORMAT_UNDEFINED;
  }
}

VkImageUsageFlagBits ImageUsage2VulkanEnum(ImageGPUUsage usage)
{
  switch (usage)
  {
    case ImageGPUUsage::Sample:
      return VK_IMAGE_USAGE_SAMPLED_BIT;
    case ImageGPUUsage::Storage:
      return VK_IMAGE_USAGE_STORAGE_BIT;
    case ImageGPUUsage::FramebufferColorAttachment:
      return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    case ImageGPUUsage::FramebufferDepthStencilAttachment:
      return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    case ImageGPUUsage::FramebufferInputAttachment:
      return VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    default:
      return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
  }
}

VkSampleCountFlagBits SampleCount2VulkanEnum(SamplesCount count)
{
  return static_cast<VkSampleCountFlagBits>(count);
}

std::tuple<VkImage, VmaAllocation, VmaAllocationInfo> CreateVMAImage(
  VmaAllocator allocator, VmaAllocationCreateFlags flags, const ImageCreateArguments & args,
  VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO)
{
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = ImageType2VulkanEnum(args.type);
  imageInfo.extent.width = args.width;
  imageInfo.extent.height = args.height;
  imageInfo.extent.depth = args.depth;
  imageInfo.mipLevels = args.mipLevels;
  imageInfo.arrayLayers = 1;
  imageInfo.format = ImageFormat2VulkanEnum(args.format);
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = ImageUsage2VulkanEnum(args.usage);
  imageInfo.samples = SampleCount2VulkanEnum(args.samples);
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

vk::ImageView CreateImageView(const vk::Device & device, const IImageGPU & image)
{
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = reinterpret_cast<VkImage>(image.GetHandle());
  viewInfo.viewType = ImageType2VulkanImageViewType(image.GetImageType());
  viewInfo.format = ImageFormat2VulkanEnum(image.GetImageFormat());
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView view;
  if (auto res = vkCreateImageView(device, &viewInfo, nullptr, &view); res != VK_SUCCESS)
    throw std::invalid_argument("Failed to create image view!");

  return vk::ImageView(view);
}

vk::Sampler CreateSampler(const vk::Device & device)
{
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 0;

  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  VkSampler resultSampler;
  if (vkCreateSampler(device, &samplerInfo, nullptr, &resultSampler) != VK_SUCCESS)
    throw std::invalid_argument("failed to create texture sampler!");
  return vk::Sampler(resultSampler);
}


} // namespace details

ImageGPU::ImageGPU(const Context & ctx, BuffersAllocator & allocator,
                   const ImageCreateArguments & args)
  : BufferBase(allocator)
  , m_owner(ctx)
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
}

ImageGPU::~ImageGPU()
{
  auto allocatorHandle = reinterpret_cast<VmaAllocator>(m_allocator.GetHandle());
  if (m_image != VK_NULL_HANDLE)
    vmaDestroyImage(allocatorHandle, m_image, reinterpret_cast<VmaAllocation>(m_memBlock));
}

InternalObjectHandle ImageGPU::GetHandle() const noexcept
{
  return static_cast<VkImage>(m_image);
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


ImageGPU_View::ImageGPU_View(const Context & ctx)
  : m_context(ctx)
{
}

ImageGPU_View::~ImageGPU_View()
{
  if (m_view)
    vkDestroyImageView(m_context.GetDevice(), m_view, nullptr);
}

ImageGPU_View::ImageGPU_View(ImageGPU_View && rhs) noexcept
  : m_context(rhs.m_context)
{
  if (this != &rhs)
  {
    std::swap(m_view, rhs.m_view);
  }
}

ImageGPU_View & ImageGPU_View::operator=(ImageGPU_View && rhs) noexcept
{
  if (this != &rhs && &m_context == &rhs.m_context)
  {
    std::swap(m_view, rhs.m_view);
  }
  return *this;
}

void ImageGPU_View::AssignImage(const IImageGPU & image)
{
  auto new_view = details::CreateImageView(m_context.GetDevice(), image);
  if (m_view)
    vkDestroyImageView(m_context.GetDevice(), m_view, nullptr);
  m_view = new_view;
}

bool ImageGPU_View::IsImageAssigned() const noexcept
{
  return m_view;
}

InternalObjectHandle ImageGPU_View::GetHandle() const noexcept
{
  return m_view;
}

ImageGPU_Sampler::ImageGPU_Sampler(const Context & ctx)
  : m_context(ctx)
{
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(ctx.GetGPU(), &properties);
}

ImageGPU_Sampler::~ImageGPU_Sampler()
{
  if (m_sampler)
    vkDestroySampler(m_context.GetDevice(), m_sampler, nullptr);
}

InternalObjectHandle ImageGPU_Sampler::GetHandle() const noexcept
{
  return m_sampler;
}

void ImageGPU_Sampler::Invalidate()
{
  if (m_invalidSampler || !m_sampler)
  {
    auto new_sampler = details::CreateSampler(m_context.GetDevice());
    if (m_sampler)
      vkDestroySampler(m_context.GetDevice(), m_sampler, nullptr);
    m_sampler = new_sampler;
    m_invalidSampler = false;
  }
}

} // namespace RHI::vulkan
