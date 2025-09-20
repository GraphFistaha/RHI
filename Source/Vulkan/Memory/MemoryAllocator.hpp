#pragma once
#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "MemoryBlock.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan::memory
{

struct MemoryAllocator final : public OwnedBy<Context>
{
  using AllocatorHandle = void *;

  explicit MemoryAllocator(Context & ctx);
  ~MemoryAllocator();
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  RESTRICTED_COPY(MemoryAllocator);

public:
  AllocatorHandle GetHandle() const noexcept { return m_allocator; }

  MemoryBlock AllocBuffer(size_t size, VkBufferUsageFlags usage, bool allowHostAccess);

  MemoryBlock AllocImage(const ImageCreateArguments & description, VkImageUsageFlags usage,
                         VkSampleCountFlagBits samples,
                         VkSharingMode shareMode = VK_SHARING_MODE_EXCLUSIVE);

private:
  AllocatorHandle m_allocator;
};
} // namespace RHI::vulkan::memory
