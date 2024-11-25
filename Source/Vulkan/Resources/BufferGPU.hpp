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
} // namespace RHI::vulkan

namespace RHI::vulkan
{
struct Transferer;

struct BufferBase
{
  using AllocInfoRawMemory = std::array<uint32_t, 14>;
  Transferer & m_transferer;
  BuffersAllocator & m_allocator;
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
  explicit BufferBase(BuffersAllocator & allocator, Transferer & transferer);
  virtual ~BufferBase() = default;
};

} // namespace RHI::vulkan

namespace RHI::vulkan
{
struct BufferGPU : public IBufferGPU,
                   private BufferBase
{
  using IBufferGPU::ScopedPointer;

  explicit BufferGPU(size_t size, VkBufferUsageFlags usage, BuffersAllocator & allocator,
                     Transferer & transferer, bool mapped = false);
  virtual ~BufferGPU() override;

  virtual void UploadSync(const void * data, size_t size, size_t offset = 0) override
  {
    return BufferBase::UploadSync(data, size, offset);
  }
  virtual void UploadAsync(const void * data, size_t size, size_t offset = 0) override;
  virtual ScopedPointer Map() override { return BufferBase::Map(); }
  virtual void Flush() const noexcept override { BufferBase::Flush(); }
  virtual bool IsMapped() const noexcept override { return BufferBase::IsMapped(); }
  virtual size_t Size() const noexcept override { return BufferBase::Size(); }

public:
  VkBuffer GetHandle() const noexcept;

private:
  vk::Buffer m_buffer = nullptr;
};
} // namespace RHI::vulkan
