#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "MemoryBlock.hpp"

namespace RHI::vulkan::memory
{

struct MemoryAllocator final
{
  using AllocatorHandle = void *;

  explicit MemoryAllocator(VkInstance instance, VkPhysicalDevice gpu, VkDevice device,
                           uint32_t vulkanVersion);
  ~MemoryAllocator();

public:
  AllocatorHandle GetHandle() const noexcept { return m_allocator; }

  MemoryBlock AllocBuffer(size_t size, VkBufferUsageFlags usage, bool allowHostAccess) const;

  MemoryBlock AllocImage(const ImageCreateArguments & description, VkImageUsageFlags usage,
                         VkSampleCountFlagBits samples) const;

private:
  AllocatorHandle m_allocator;
};
} // namespace RHI::vulkan::memory
