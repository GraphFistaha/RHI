#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "BuffersAllocator.hpp"

namespace RHI::vulkan
{
struct Transferer;
}

namespace RHI::vulkan::details
{
struct BufferBase
{
  using AllocInfoRawMemory = std::array<uint32_t, 14>;

  Transferer * m_transferer = nullptr;
  const details::BuffersAllocator & m_allocator;

  AllocInfoRawMemory m_allocInfo;
  InternalObjectHandle m_memBlock = nullptr;
  uint32_t m_flags = 0;
  size_t m_size = 0;

  void UploadSync(const void * data, size_t size, size_t offset = 0);
  IBufferGPU::ScopedPointer Map();
  void Flush() const noexcept;
  bool IsMapped() const noexcept;
  size_t Size() const noexcept { return m_size; }



protected:
  explicit BufferBase(const details::BuffersAllocator & allocator, Transferer * transferer);
  virtual ~BufferBase() = default;
  BufferBase(BufferBase && rhs) noexcept;
  BufferBase & operator=(BufferBase && rhs) noexcept;
};

} // namespace RHI::vulkan::details
