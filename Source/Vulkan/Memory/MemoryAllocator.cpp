#include "MemoryAllocator.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan::memory
{
MemoryAllocator::MemoryAllocator(Context & ctx)
  : OwnedBy<Context>(ctx)
{
  static_assert(
    sizeof(VmaAllocationInfo) <= sizeof(MemoryBlock::AllocInfoRawMemory),
    "size of BufferBase::AllocInfoRawMemory less then should be. It should be at least as VmaAllocationInfo");

  VmaAllocatorCreateInfo allocator_info{};
  allocator_info.instance = ctx.GetInstance();
  allocator_info.physicalDevice = ctx.GetGPU();
  allocator_info.device = ctx.GetDevice();

  //Validation layers causes crash in GetBufferMemoryRequirements
#ifdef ENABLE_VALIDATION_LAYERS
  allocator_info.vulkanApiVersion = VK_API_VERSION_1_0;
#else
  allocator_info.vulkanApiVersion = ctx.GetVulkanVersion();
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
                                         bool allowHostAccess)
{
  return MemoryBlock(*this, size, usage, allowHostAccess);
}

MemoryBlock MemoryAllocator::AllocImage(const ImageCreateArguments & description,
                                        VkImageUsageFlags usage, VkSampleCountFlagBits samples, VkSharingMode shareMode)
{
  return MemoryBlock(*this, description, usage, samples, shareMode);
}

} // namespace RHI::vulkan::memory
