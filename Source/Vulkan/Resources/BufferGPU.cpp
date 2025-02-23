#include "BufferGPU.hpp"

#include <vk_mem_alloc.h>

#include "../VulkanContext.hpp"
#include "Transferer.hpp"


namespace RHI::vulkan
{

BufferGPU::BufferGPU(Context & ctx, size_t size, VkBufferUsageFlags usage, bool mapped /* = false*/)
  : OwnedBy<Context>(ctx)
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

  m_memBlock =
    GetContext().GetBuffersAllocator().AllocBuffer(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                   allocation_flags, VMA_MEMORY_USAGE_AUTO);
}

BufferGPU::~BufferGPU()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(std::move(m_memBlock), nullptr);
}

BufferGPU::BufferGPU(BufferGPU && rhs) noexcept
  : OwnedBy<Context>(std::move(rhs))
{
  std::swap(m_memBlock, rhs.m_memBlock);
}

BufferGPU & BufferGPU::operator=(BufferGPU && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<Context>::operator=(std::move(rhs));
    std::swap(m_memBlock, rhs.m_memBlock);
  }
  return *this;
}

void BufferGPU::UploadSync(const void * data, size_t size, size_t offset)
{
  m_memBlock.UploadSync(data, size, offset);
}

std::future<UploadResult> BufferGPU::UploadAsync(const void * data, size_t size, size_t offset)
{
  return GetContext().GetTransferer().UploadBuffer(*this, reinterpret_cast<const uint8_t *>(data),
                                                   size, offset);
}

IBufferGPU::ScopedPointer BufferGPU::Map()
{
  return m_memBlock.Map();
}

void BufferGPU::Flush() const noexcept
{
  m_memBlock.Flush();
}

bool BufferGPU::IsMapped() const noexcept
{
  return m_memBlock.IsMapped();
}

size_t BufferGPU::Size() const noexcept
{
  return m_memBlock.Size();
}

VkBuffer BufferGPU::GetHandle() const noexcept
{
  return m_memBlock.GetBuffer();
}
} // namespace RHI::vulkan
