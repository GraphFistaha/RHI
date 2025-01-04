#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::details
{
struct BuffersAllocator final
{
  using AllocInfoRawMemory = std::array<uint32_t, 14>;
  using AllocationHandle = void *;
  using AllocatorHandle = void *;

  explicit BuffersAllocator(VkInstance instance, VkPhysicalDevice gpu, VkDevice device,
                            uint32_t vulkanVersion);
  ~BuffersAllocator();

  AllocatorHandle GetHandle() const noexcept { return m_allocator; }

  std::tuple<VkBuffer, AllocationHandle, AllocInfoRawMemory> AllocBuffer(
    size_t size, VkBufferUsageFlags usage, uint32_t flags, uint32_t memoryUsage) const;

  void FreeBuffer(VkBuffer buffer, AllocationHandle allocation) const;

  std::tuple<VkImage, AllocationHandle, AllocInfoRawMemory> AllocImage(
    const ImageCreateArguments & args, uint32_t flags, uint32_t memoryUsage) const;

  void FreeImage(VkImage image, AllocationHandle allocation) const;

private:
  AllocatorHandle m_allocator;
};
} // namespace RHI::vulkan::details
