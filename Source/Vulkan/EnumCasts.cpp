#pragma once

#include "VulkanContext.hpp"

namespace RHI::vulkan::utils
{
template<>
VkIndexType CastInterfaceEnum2Vulkan<VkIndexType, RHI::IndexType>(RHI::IndexType type)
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

template<>
VkBufferUsageFlags CastInterfaceEnum2Vulkan<VkBufferUsageFlags, RHI::BufferGPUUsage>(
  RHI::BufferGPUUsage usage)
{
  switch (usage)
  {
    case BufferGPUUsage::VertexBuffer:
      return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    case BufferGPUUsage::IndexBuffer:
      return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    case BufferGPUUsage::UniformBuffer:
      return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    case BufferGPUUsage::StorageBuffer:
      return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    case BufferGPUUsage::TransformFeedbackBuffer:
      return VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
    case BufferGPUUsage::IndirectBuffer:
      return VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    default:
      throw std::range_error("Failed to cast usage in Vulkan enum");
  }
}

template<>
VkImageType CastInterfaceEnum2Vulkan<VkImageType, ImageType>(ImageType type)
{
  return static_cast<VkImageType>(type);
}

template<>
VkImageViewType CastInterfaceEnum2Vulkan<VkImageViewType, ImageType>(ImageType type)
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

template<>
VkFormat CastInterfaceEnum2Vulkan<VkFormat, ImageFormat>(ImageFormat format)
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

template<>
VkImageUsageFlags CastInterfaceEnum2Vulkan<VkImageUsageFlags, ImageGPUUsage>(
  ImageGPUUsage usage)
{
  switch (usage)
  {
    case ImageGPUUsage::Sample:
      return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    case ImageGPUUsage::Storage:
      return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    case ImageGPUUsage::FramebufferColorAttachment:
      return VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    case ImageGPUUsage::FramebufferDepthStencilAttachment:
      return VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    case ImageGPUUsage::FramebufferInputAttachment:
      return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    default:
      return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
  }
}

template<>
VkSampleCountFlagBits CastInterfaceEnum2Vulkan<VkSampleCountFlagBits, SamplesCount>(
  SamplesCount count)
{
  return static_cast<VkSampleCountFlagBits>(count);
}
} // namespace RHI::vulkan::utils
