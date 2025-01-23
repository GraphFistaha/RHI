#include "MemoryBlock.hpp"

#include <vk_mem_alloc.h>

#include "../Utils/CastHelper.hpp"
#include "BuffersAllocator.hpp"

namespace RHI::vulkan::memory
{

MemoryBlock::MemoryBlock(InternalObjectHandle allocator, const ImageDescription & description,
                         uint32_t flags, uint32_t memoryUsage)
  : m_allocator(allocator)
{
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = utils::CastInterfaceEnum2Vulkan<VkImageType>(description.type);
  imageInfo.extent.width = description.extent[0];
  imageInfo.extent.height = description.extent[1];
  imageInfo.extent.depth = description.extent[2];
  imageInfo.mipLevels = description.mipLevels;
  imageInfo.arrayLayers = 1;
  imageInfo.format = utils::CastInterfaceEnum2Vulkan<VkFormat>(description.format);
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = utils::CastInterfaceEnum2Vulkan<VkImageUsageFlags>(description.format);
  imageInfo.samples = utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(description.samples);
  imageInfo.sharingMode = description.shared ? VK_SHARING_MODE_CONCURRENT
                                                : VK_SHARING_MODE_EXCLUSIVE;

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

  m_image = image;
  m_memBlock = allocation;
  m_allocInfo = reinterpret_cast<AllocInfoRawMemory &>(allocInfo);
  m_flags = flags;
  m_size = allocInfo.size;
}

MemoryBlock::MemoryBlock(InternalObjectHandle allocator, size_t size, VkBufferUsageFlags usage,
                         uint32_t flags, uint32_t memoryUsage)
  : m_allocator(allocator)
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

  m_buffer = buffer;
  m_memBlock = allocation;
  m_allocInfo = reinterpret_cast<AllocInfoRawMemory &>(allocInfo);
  m_size = allocInfo.size;
  m_flags = flags;
}

MemoryBlock::MemoryBlock(MemoryBlock && rhs) noexcept
{
  std::swap(m_allocator, rhs.m_allocator);
  std::swap(m_buffer, rhs.m_buffer);
  std::swap(m_image, rhs.m_image);
  std::swap(m_allocInfo, rhs.m_allocInfo);
  std::swap(m_flags, rhs.m_flags);
  std::swap(m_memBlock, rhs.m_memBlock);
  std::swap(m_size, rhs.m_size);
}

MemoryBlock & MemoryBlock::operator=(MemoryBlock && rhs) noexcept
{
  if (this != &rhs)
  {
    std::swap(m_allocator, rhs.m_allocator);
    std::swap(m_buffer, rhs.m_buffer);
    std::swap(m_image, rhs.m_image);
    std::swap(m_allocInfo, rhs.m_allocInfo);
    std::swap(m_flags, rhs.m_flags);
    std::swap(m_memBlock, rhs.m_memBlock);
    std::swap(m_size, rhs.m_size);
  }
  return *this;
}

MemoryBlock::operator bool() const noexcept
{
  return true;
}

MemoryBlock::~MemoryBlock()
{
  auto * allocatorHandle = reinterpret_cast<VmaAllocator>(m_allocator);
  if (m_buffer)
    vmaDestroyBuffer(allocatorHandle, m_buffer, reinterpret_cast<VmaAllocation>(m_memBlock));
  if (m_image)
    vmaDestroyImage(allocatorHandle, m_image, reinterpret_cast<VmaAllocation>(m_memBlock));
}

void MemoryBlock::UploadSync(const void * data, size_t size, size_t offset)
{
  auto allocator = reinterpret_cast<VmaAllocator>(m_allocator);
  auto allocation = reinterpret_cast<VmaAllocation>(m_memBlock);
  vmaCopyMemoryToAllocation(allocator, data, allocation, offset, size);
}

IBufferGPU::ScopedPointer MemoryBlock::Map()
{
  void * mapped_memory = nullptr;
  const bool is_mapped = IsMapped();
  auto allocator = reinterpret_cast<VmaAllocator>(m_allocator);
  if (!is_mapped)
  {
    if (VkResult res =
          vmaMapMemory(allocator, reinterpret_cast<VmaAllocation>(m_memBlock), &mapped_memory);
        res != VK_SUCCESS)
      throw std::runtime_error("Failed to map buffer memory");
  }
  else
  {
    mapped_memory = reinterpret_cast<const VmaAllocationInfo &>(m_allocInfo).pMappedData;
  }

  return IBufferGPU::ScopedPointer(reinterpret_cast<char *>(mapped_memory),
                                   [this, is_mapped, allocator](char * mem_ptr)
                                   {
                                     if (!is_mapped && mem_ptr != nullptr)
                                       vmaUnmapMemory(allocator,
                                                      reinterpret_cast<VmaAllocation>(m_memBlock));
                                   });
}

void MemoryBlock::Flush() const noexcept
{
  auto allocator = reinterpret_cast<VmaAllocator>(m_allocator);
  vmaFlushAllocation(allocator, reinterpret_cast<VmaAllocation>(m_memBlock), 0, m_size);
}


bool MemoryBlock::IsMapped() const noexcept
{
  return (m_flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0;
}

} // namespace RHI::vulkan::memory
