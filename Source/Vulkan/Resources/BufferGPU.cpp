#include "BufferGPU.hpp"

#include <vk_mem_alloc.h>

#include "../VulkanContext.hpp"
#include "Transferer.hpp"


namespace RHI::vulkan
{
namespace details
{

std::tuple<VkBuffer, VmaAllocation, VmaAllocationInfo> CreateVMABuffer(
  VmaAllocator allocator, size_t size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags,
  VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO)
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;

  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = memory_usage;
  allocCreateInfo.flags = flags;
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo allocInfo;

  if (auto res =
        vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &buffer, &allocation, &allocInfo);
      res != VK_SUCCESS)
    throw std::invalid_argument("Failed to create Buffer");
  return {buffer, allocation, allocInfo};
}
} // namespace details

BufferGPU::BufferGPU(size_t size, VkBufferUsageFlags usage,
                     const details::BuffersAllocator & allocator,
                     Transferer * transferer /*= nullptr*/, bool mapped /* = false*/)
  : BufferBase(allocator, transferer)
{
  VmaAllocationCreateFlags allocation_flags = 0;
  if (mapped)
  {
    allocation_flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                       VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
  }
  else
  {
    allocation_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  }

  auto allocatorHandle = reinterpret_cast<VmaAllocator>(m_allocator.GetHandle());
  auto [buffer, allocation, alloc_info] =
    details::CreateVMABuffer(allocatorHandle, size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, allocation_flags);
  m_buffer = buffer;
  m_memBlock = allocation;
  m_allocInfo = reinterpret_cast<BufferGPU::AllocInfoRawMemory &>(alloc_info);
  m_size = size;
  m_flags = allocation_flags;
}

BufferGPU::~BufferGPU()
{
  auto allocatorHandle = reinterpret_cast<VmaAllocator>(m_allocator.GetHandle());
  if (!!m_buffer)
    vmaDestroyBuffer(allocatorHandle, m_buffer, reinterpret_cast<VmaAllocation>(m_memBlock));
}

BufferGPU::BufferGPU(BufferGPU && rhs) noexcept
  : BufferBase(std::move(rhs))
{
  std::swap(m_buffer, rhs.m_buffer);
}

BufferGPU & BufferGPU::operator=(BufferGPU && rhs) noexcept
{
  if (this != &rhs)
  {
    std::swap(m_buffer, rhs.m_buffer);
    BufferBase::operator=(std::move(rhs));
  }
  return *this;
}

void BufferGPU::UploadAsync(const void * data, size_t size, size_t offset)
{
  if (!m_transferer)
    throw std::runtime_error("This buffer isn't appropriate for async uploading. Use UploadSync or mapping");
  BufferGPU stagingBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, m_allocator);
  stagingBuffer.UploadSync(data, size, offset);
  m_transferer->UploadBuffer(m_buffer, std::move(stagingBuffer));
}

VkBuffer BufferGPU::GetHandle() const noexcept
{
  return static_cast<VkBuffer>(m_buffer);
}
} // namespace RHI::vulkan
