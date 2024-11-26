#include "BufferBase.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace RHI::vulkan::details
{

void BufferBase::UploadSync(const void * data, size_t size, size_t offset)
{
  auto allocator = reinterpret_cast<VmaAllocator>(m_allocator.GetHandle());
  auto allocation = reinterpret_cast<VmaAllocation>(m_memBlock);
  vmaCopyMemoryToAllocation(allocator, data, allocation, offset, size);
}

RHI::IBufferGPU::ScopedPointer BufferBase::Map()
{
  void * mapped_memory = nullptr;
  const bool is_mapped = IsMapped();
  auto allocatorHandle = reinterpret_cast<VmaAllocator>(m_allocator.GetHandle());
  if (!is_mapped)
  {
    if (VkResult res = vmaMapMemory(allocatorHandle, reinterpret_cast<VmaAllocation>(m_memBlock),
                                    &mapped_memory);
        res != VK_SUCCESS)
      throw std::runtime_error("Failed to map buffer memory");
  }
  else
  {
    mapped_memory = reinterpret_cast<const VmaAllocationInfo &>(m_allocInfo).pMappedData;
  }

  return IBufferGPU::ScopedPointer(reinterpret_cast<char *>(mapped_memory),
                                   [this, is_mapped, allocatorHandle](char * mem_ptr)
                                   {
                                     if (!is_mapped && mem_ptr != nullptr)
                                       vmaUnmapMemory(allocatorHandle,
                                                      reinterpret_cast<VmaAllocation>(m_memBlock));
                                   });
}

void BufferBase::Flush() const noexcept
{
  auto allocatorHandle = reinterpret_cast<VmaAllocator>(m_allocator.GetHandle());
  vmaFlushAllocation(allocatorHandle, reinterpret_cast<VmaAllocation>(m_memBlock), 0, m_size);
}

bool BufferBase::IsMapped() const noexcept
{
  return (m_flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0;
}

BufferBase::BufferBase(const details::BuffersAllocator & allocator, Transferer * transferer)
  : m_allocator(allocator)
  , m_transferer(transferer)
{
  static_assert(
    sizeof(VmaAllocationInfo) <= sizeof(AllocInfoRawMemory),
    "size of BufferBase::AllocInfoRawMemory less then should be. It should be at least as VmaAllocationInfo");
}

BufferBase::BufferBase(BufferBase && rhs) noexcept
  : m_allocator(rhs.m_allocator)
{
  std::swap(m_transferer, rhs.m_transferer);
  std::swap(m_allocInfo, rhs.m_allocInfo);
  std::swap(m_flags, rhs.m_flags);
  std::swap(m_memBlock, rhs.m_memBlock);
  std::swap(m_size, rhs.m_size);
}

BufferBase & BufferBase::operator=(BufferBase && rhs) noexcept
{
  if (this != &rhs && &m_allocator == &rhs.m_allocator)
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
