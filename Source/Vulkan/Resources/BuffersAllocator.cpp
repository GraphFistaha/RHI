#include "BuffersAllocator.hpp"

#include <vk_mem_alloc.h>

namespace RHI::vulkan::details
{
BuffersAllocator::BuffersAllocator(VkInstance instance, VkPhysicalDevice gpu, VkDevice device,
                                   uint32_t vulkanVersion)
{
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

BuffersAllocator::~BuffersAllocator()
{
  vmaDestroyAllocator(reinterpret_cast<VmaAllocator>(m_allocator));
}
}