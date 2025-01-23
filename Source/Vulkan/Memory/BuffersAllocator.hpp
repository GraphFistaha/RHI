#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "MemoryBlock.hpp"

namespace RHI::vulkan::memory
{

struct BuffersAllocator final
{
  using AllocatorHandle = void *;

  explicit BuffersAllocator(VkInstance instance, VkPhysicalDevice gpu, VkDevice device,
                            uint32_t vulkanVersion);
  ~BuffersAllocator();

  AllocatorHandle GetHandle() const noexcept { return m_allocator; }

  MemoryBlock AllocBuffer(size_t size, VkBufferUsageFlags usage, uint32_t flags,
                          uint32_t memoryUsage) const;

  MemoryBlock AllocImage(const ImageDescription & description, uint32_t flags,
                         uint32_t memoryUsage) const;

private:
  AllocatorHandle m_allocator;
};
} // namespace RHI::vulkan::memory
