#pragma once
#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::memory
{
struct MemoryAllocator;
}

namespace RHI::vulkan::memory
{
struct MemoryBlock final : public OwnedBy<MemoryAllocator>
{
  MemoryBlock() = default;
  virtual ~MemoryBlock() override;
  MemoryBlock(MemoryBlock && rhs) noexcept;
  MemoryBlock & operator=(MemoryBlock && rhs) noexcept;
  MAKE_ALIAS_FOR_GET_OWNER(MemoryAllocator, GetAllocator);
  RESTRICTED_COPY(MemoryBlock);

private:
  /// create memory block for image
  explicit MemoryBlock(MemoryAllocator & allocator, const ImageCreateArguments & description,
                       VkImageUsageFlags usage, VkSampleCountFlagBits samples);
  /// Create memory block for buffer
  explicit MemoryBlock(MemoryAllocator & allocator, size_t size, VkBufferUsageFlags usage,
                       bool allowHostAccess);

public:
  bool UploadSync(const void * data, size_t size, size_t offset = 0);
  bool DownloadSync(size_t offset, void * data, size_t size) const;
  IBufferGPU::ScopedPointer Map();
  void Flush() const noexcept;
  bool IsMapped() const noexcept;
  size_t Size() const noexcept { return m_size; }
  VkImage GetImage() const noexcept { return m_image; }
  VkBuffer GetBuffer() const noexcept { return m_buffer; }
  operator bool() const noexcept;

private:
  friend struct MemoryAllocator;
  using AllocInfoRawMemory = std::array<uint32_t, 16>;

  AllocInfoRawMemory m_allocInfo{};
  InternalObjectHandle m_memBlock = nullptr;
  VkImage m_image = VK_NULL_HANDLE;
  VkBuffer m_buffer = VK_NULL_HANDLE;
  size_t m_size = 0;
  uint32_t m_flags = 0;
};

} // namespace RHI::vulkan::memory
