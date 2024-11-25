#include "BufferGPU.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "Transferer.hpp"

namespace RHI::vulkan
{
BuffersAllocator::BuffersAllocator(const Context & ctx)
{
  VmaAllocatorCreateInfo allocator_info{};
  allocator_info.instance = ctx.GetInstance();
  allocator_info.physicalDevice = ctx.GetGPU();
  allocator_info.device = ctx.GetDevice();

  //Validation layers causes crash in GetBufferMemoryRequirements
#ifdef ENABLE_VALIDATION_LAYERS
  allocator_info.vulkanApiVersion = VK_API_VERSION_1_0; //ctx.GetVulkanVersion();
#else
  allocator_info.vulkanApiVersion = ctx.GetVulkanVersion();
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

void BufferBase::UploadSync(const void * data, size_t size, size_t offset)
{
  auto allocator = reinterpret_cast<VmaAllocator>(m_allocator.GetHandle());
  auto allocation = reinterpret_cast<VmaAllocation>(m_memBlock);
  vmaCopyMemoryToAllocation(allocator, data, allocation, offset, size);
}

IBufferGPU::ScopedPointer BufferBase::Map()
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

  return BufferGPU::ScopedPointer(reinterpret_cast<char *>(mapped_memory),
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
BufferBase::BufferBase(BuffersAllocator & allocator, Transferer & transferer)
  : m_allocator(allocator)
  , m_transferer(transferer)
{
  static_assert(
    sizeof(VmaAllocationInfo) <= sizeof(AllocInfoRawMemory),
    "size of BufferBase::AllocInfoRawMemory less then should be. It should be at least as VmaAllocationInfo");
}
} // namespace RHI::vulkan

namespace RHI::vulkan
{
namespace details
{
VkBufferUsageFlags UsageToVulkanEnum(BufferGPUUsage usage)
{
  switch (usage)
  {
    case BufferGPUUsage::VertexBuffer:
      return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    case BufferGPUUsage::IndexBuffer:
      return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    case BufferGPUUsage::UniformBuffer:
      return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    case BufferGPUUsage::StorageBuffer:
      return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    case BufferGPUUsage::TransformFeedbackBuffer:
      return VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
    case BufferGPUUsage::IndirectBuffer:
      return VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    default:
      throw std::range_error("Failed to cast usage in Vulkan enum");
  }
}


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

BufferGPU::BufferGPU(size_t size, VkBufferUsageFlags usage, BuffersAllocator & allocator, Transferer & transferer,
                     bool mapped /* = false*/)
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
    details::CreateVMABuffer(allocatorHandle, size, usage, allocation_flags);
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

void BufferGPU::UploadAsync(const void * data, size_t size, size_t offset)
{
  BufferGPU stagingBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, m_allocator, m_transferer);
  stagingBuffer.UploadSync(data, size, offset);
  m_transferer.UploadBuffer(m_buffer, std::move(stagingBuffer));
}

VkBuffer BufferGPU::GetHandle() const noexcept
{
  return static_cast<VkBuffer>(m_buffer);
}
} // namespace RHI::vulkan
