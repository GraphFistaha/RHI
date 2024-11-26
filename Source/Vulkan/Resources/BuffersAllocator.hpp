#pragma once
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::details
{
struct BuffersAllocator final
{
  using AllocatorHandle = void *;
  explicit BuffersAllocator(VkInstance instance, VkPhysicalDevice gpu, VkDevice device, uint32_t vulkanVersion);
  ~BuffersAllocator();

  AllocatorHandle GetHandle() const noexcept { return m_allocator; }

private:
  AllocatorHandle m_allocator;
};
} // namespace RHI::vulkan::details
