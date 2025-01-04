#include "BufferGPU.hpp"

#include <vk_mem_alloc.h>

#include "../VulkanContext.hpp"


namespace RHI::vulkan
{

BufferGPU::BufferGPU(const Context & ctx, Transferer * transferer, size_t size,
                     VkBufferUsageFlags usage, bool mapped /* = false*/)
  : BufferBase(ctx, transferer)
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

  std::tie(m_buffer, m_memBlock, m_allocInfo) =
    m_context.GetBuffersAllocator().AllocBuffer(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                allocation_flags, VMA_MEMORY_USAGE_AUTO);
  m_size = size;
  m_flags = allocation_flags;
}

BufferGPU::~BufferGPU()
{
  m_context.GetGarbageCollector().PushVkObjectToDestroy(m_buffer, m_memBlock);
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
  BufferGPU stagingBuffer(m_context, nullptr, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  stagingBuffer.UploadSync(data, size, offset);
  m_transferer->UploadBuffer(this, std::move(stagingBuffer));
}

VkBuffer BufferGPU::GetHandle() const noexcept
{
  return static_cast<VkBuffer>(m_buffer);
}
} // namespace RHI::vulkan
