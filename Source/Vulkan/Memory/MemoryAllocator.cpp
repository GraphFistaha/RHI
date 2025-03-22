#include "MemoryAllocator.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "../Utils/CastHelper.hpp"

namespace RHI::vulkan::memory
{
MemoryAllocator::MemoryAllocator(VkInstance instance, VkPhysicalDevice gpu, VkDevice device,
                                 uint32_t vulkanVersion)
{
  static_assert(
    sizeof(VmaAllocationInfo) <= sizeof(MemoryBlock::AllocInfoRawMemory),
    "size of BufferBase::AllocInfoRawMemory less then should be. It should be at least as VmaAllocationInfo");

  VmaAllocatorCreateInfo allocator_info{};
  allocator_info.instance = instance;
  allocator_info.physicalDevice = gpu;
  allocator_info.device = device;

  //Validation layers causes crash in GetBufferMemoryRequirements
#ifdef ENABLE_VALIDATION_LAYERS
  allocator_info.vulkanApiVersion = VK_API_VERSION_1_0; //ctx.GetVulkanVersion();
#else
  allocator_info.vulkanApiVersion = vulkanVersion;
#endif

  VmaAllocator allocator;
  if (auto res = vmaCreateAllocator(&allocator_info, &allocator); res != VK_SUCCESS)
    throw std::invalid_argument("Failed to create buffers allocator");
  m_allocator = allocator;
}

MemoryAllocator::~MemoryAllocator()
{
  vmaDestroyAllocator(reinterpret_cast<VmaAllocator>(m_allocator));
}

MemoryBlock MemoryAllocator::AllocBuffer(size_t size, VkBufferUsageFlags usage,
                                         bool allowHostAccess) const
{
  return MemoryBlock(m_allocator, size, usage, allowHostAccess);
}

MemoryBlock MemoryAllocator::AllocImage(const ImageCreateArguments & description,
                                        VkImageUsageFlags usage) const
{
  return MemoryBlock(m_allocator, description, usage);
}

} // namespace RHI::vulkan::memory
