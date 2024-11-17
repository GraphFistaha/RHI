#pragma once
#include "../VulkanContext.hpp"

namespace RHI::vulkan
{
struct BuffersAllocator final
{
  explicit BuffersAllocator(const Context & ctx);
  ~BuffersAllocator();

  InternalObjectHandle GetHandle() const noexcept { return m_allocator; }

private:
  InternalObjectHandle m_allocator;
};

struct BufferBase
{
  using AllocInfoRawMemory = std::array<uint32_t, 14>;
  BuffersAllocator & m_allocator;
  AllocInfoRawMemory m_allocInfo;
  InternalObjectHandle m_memBlock = nullptr;
  uint32_t m_flags = 0;
  size_t m_size = 0;

  void Upload(const void * data, size_t size, size_t offset = 0);
  IBufferGPU::ScopedPointer Map();
  void Flush() const noexcept;
  bool IsMapped() const noexcept;
  size_t Size() const noexcept { return m_size; }

protected:
  explicit BufferBase(BuffersAllocator & allocator);
  virtual ~BufferBase() = default;
};

struct BufferGPU : public IBufferGPU,
                   private BufferBase
{
  using IBufferGPU::ScopedPointer;

  explicit BufferGPU(size_t size, BufferGPUUsage usage, BuffersAllocator & allocator,
                     bool mapped = false);
  virtual ~BufferGPU() override;

  virtual void Upload(const void * data, size_t size, size_t offset = 0) override
  {
    return BufferBase::Upload(data, size, offset);
  }
  virtual ScopedPointer Map() override { return BufferBase::Map(); }
  virtual void Flush() const noexcept override { BufferBase::Flush(); }
  virtual bool IsMapped() const noexcept override { return BufferBase::IsMapped(); }
  virtual InternalObjectHandle GetHandle() const noexcept override;
  virtual size_t Size() const noexcept override { return BufferBase::Size(); }

private:
  vk::Buffer m_buffer = nullptr;
};
} // namespace RHI::vulkan
