#include "BufferGPU.hpp"

#include "../VulkanContext.hpp"
#include "Transferer.hpp"

namespace
{
constexpr VkBufferUsageFlags Cast2VkBufferUsage(RHI::BufferGPUUsage usage) noexcept
{
  VkBufferUsageFlags result = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  if (usage & RHI::BufferGPUUsage::VertexBuffer)
    result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  else if (usage & RHI::BufferGPUUsage::IndexBuffer)
    result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

  if (usage & RHI::BufferGPUUsage::UniformBuffer)
    result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  if (usage & RHI::BufferGPUUsage::StorageBuffer)
    result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  if (usage & RHI::BufferGPUUsage::TransformFeedbackBuffer)
    result |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
  if (usage & RHI::BufferGPUUsage::IndirectBuffer)
    result |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
  return result;
}
} // namespace

namespace RHI::vulkan
{
BufferGPU::BufferGPU(Context & ctx, size_t size, BufferGPUUsage usage, bool allowHostAccess)
  : BufferGPU(ctx, size, Cast2VkBufferUsage(usage), allowHostAccess)
{
}

BufferGPU::BufferGPU(Context & ctx, size_t size, VkBufferUsageFlags usage, bool allowHostAccess)
  : OwnedBy<Context>(ctx)
  , m_memBlock(ctx.GetBuffersAllocator().AllocBuffer(size, usage, allowHostAccess))
{
}

BufferGPU::~BufferGPU()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(std::move(m_memBlock), nullptr);
}

BufferGPU::BufferGPU(BufferGPU && rhs) noexcept
  : OwnedBy<Context>(std::move(rhs))
  , m_memBlock(std::move(rhs.m_memBlock))
{
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
  return GetContext().GetTransferer().UploadBuffer(GetHandle(),
                                                   reinterpret_cast<const uint8_t *>(data), size,
                                                   offset);
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
