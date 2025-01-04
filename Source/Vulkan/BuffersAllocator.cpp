#include "BuffersAllocator.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "Utils/CastHelper.hpp"

namespace RHI::vulkan::details
{
BuffersAllocator::BuffersAllocator(VkInstance instance, VkPhysicalDevice gpu, VkDevice device,
                                   uint32_t vulkanVersion)
{
  static_assert(
    sizeof(VmaAllocationInfo) <= sizeof(AllocInfoRawMemory),
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

BuffersAllocator::~BuffersAllocator()
{
  vmaDestroyAllocator(reinterpret_cast<VmaAllocator>(m_allocator));
}

std::tuple<VkBuffer, BuffersAllocator::AllocationHandle, BuffersAllocator::AllocInfoRawMemory>
BuffersAllocator::AllocBuffer(size_t size, VkBufferUsageFlags usage, uint32_t flags,
                              uint32_t memoryUsage) const
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;

  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = static_cast<VmaMemoryUsage>(memoryUsage);
  allocCreateInfo.flags = flags;
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo allocInfo;
  auto * allocatorHandle = reinterpret_cast<VmaAllocator>(m_allocator);

  if (auto res = vmaCreateBuffer(allocatorHandle, &bufferInfo, &allocCreateInfo, &buffer,
                                 &allocation, &allocInfo);
      res != VK_SUCCESS)
    throw std::invalid_argument("Failed to create Buffer");
  return {buffer, allocation, reinterpret_cast<AllocInfoRawMemory &>(allocInfo)};
}

void BuffersAllocator::FreeBuffer(VkBuffer buffer, AllocationHandle allocation) const
{
  auto * allocatorHandle = reinterpret_cast<VmaAllocator>(m_allocator);
  vmaDestroyBuffer(allocatorHandle, buffer, reinterpret_cast<VmaAllocation>(allocation));
}

std::tuple<VkImage, BuffersAllocator::AllocationHandle, BuffersAllocator::AllocInfoRawMemory>
BuffersAllocator::AllocImage(const ImageCreateArguments & args, uint32_t flags,
                             uint32_t memoryUsage) const
{
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = utils::CastInterfaceEnum2Vulkan<VkImageType>(args.type);
  imageInfo.extent.width = args.extent[0];
  imageInfo.extent.height = args.extent[1];
  imageInfo.extent.depth = args.extent[2];
  imageInfo.mipLevels = args.mipLevels;
  imageInfo.arrayLayers = 1;
  imageInfo.format = utils::CastInterfaceEnum2Vulkan<VkFormat>(args.format);
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = utils::CastInterfaceEnum2Vulkan<VkImageUsageFlags>(args.format);
  imageInfo.samples = utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(args.samples);
  imageInfo.sharingMode = args.shared ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = static_cast<VmaMemoryUsage>(memoryUsage);
  allocCreateInfo.flags = flags;
  allocCreateInfo.priority = 1.0f;
  VkImage image;
  VmaAllocation allocation;
  VmaAllocationInfo allocInfo;
  auto * allocatorHandle = reinterpret_cast<VmaAllocator>(m_allocator);
  if (auto res = vmaCreateImage(allocatorHandle, &imageInfo, &allocCreateInfo, &image, &allocation,
                                &allocInfo);
      res != VK_SUCCESS)
    throw std::invalid_argument("Failed to create VkImage");

  return {image, allocation, reinterpret_cast<AllocInfoRawMemory &>(allocInfo)};
}

void BuffersAllocator::FreeImage(VkImage image, AllocationHandle allocation) const
{
  auto * allocatorHandle = reinterpret_cast<VmaAllocator>(m_allocator);
  vmaDestroyImage(allocatorHandle, image, reinterpret_cast<VmaAllocation>(allocation));
}

} // namespace RHI::vulkan::details
