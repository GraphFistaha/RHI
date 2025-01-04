#include "BufferBase.hpp"

#include <vk_mem_alloc.h>

#include "../VulkanContext.hpp"

namespace RHI::vulkan::details
{

void BufferBase::UploadSync(const void * data, size_t size, size_t offset)
{
  auto allocator = reinterpret_cast<VmaAllocator>(m_context.GetBuffersAllocator().GetHandle());
  auto allocation = reinterpret_cast<VmaAllocation>(m_memBlock);
  vmaCopyMemoryToAllocation(allocator, data, allocation, offset, size);
}

RHI::IBufferGPU::ScopedPointer BufferBase::Map()
{
  void * mapped_memory = nullptr;
  const bool is_mapped = IsMapped();
  auto allocator = reinterpret_cast<VmaAllocator>(m_context.GetBuffersAllocator().GetHandle());
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

void BufferBase::Flush() const noexcept
{
  auto allocator = reinterpret_cast<VmaAllocator>(m_context.GetBuffersAllocator().GetHandle());
  vmaFlushAllocation(allocator, reinterpret_cast<VmaAllocation>(m_memBlock), 0, m_size);
}

bool BufferBase::IsMapped() const noexcept
{
  return (m_flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0;
}

BufferBase::BufferBase(const Context & ctx, Transferer * transferer)
  : m_context(ctx)
  , m_transferer(transferer)
{
}

BufferBase::BufferBase(BufferBase && rhs) noexcept
  : m_context(rhs.m_context)
{
  std::swap(m_transferer, rhs.m_transferer);
  std::swap(m_allocInfo, rhs.m_allocInfo);
  std::swap(m_flags, rhs.m_flags);
  std::swap(m_memBlock, rhs.m_memBlock);
  std::swap(m_size, rhs.m_size);
}

BufferBase & BufferBase::operator=(BufferBase && rhs) noexcept
{
  if (this != &rhs && &m_context == &rhs.m_context)
  {
    std::swap(m_transferer, rhs.m_transferer);
    std::swap(m_allocInfo, rhs.m_allocInfo);
    std::swap(m_flags, rhs.m_flags);
    std::swap(m_memBlock, rhs.m_memBlock);
    std::swap(m_size, rhs.m_size);
  }
  return *this;
}

} // namespace RHI::vulkan::details
