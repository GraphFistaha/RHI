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
} // namespace RHI::vulkan::utils
