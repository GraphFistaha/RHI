#pragma once
#include <RHI.hpp>

#include "BufferBase.hpp"

namespace RHI::vulkan
{

struct BufferGPU : public IBufferGPU,
                   private details::BufferBase
{
  using IBufferGPU::ScopedPointer;

  explicit BufferGPU(const Context & ctx, Transferer * transferer, size_t size,
                     VkBufferUsageFlags usage, bool mapped = false);
  virtual ~BufferGPU() override;

  BufferGPU(BufferGPU && rhs) noexcept;
  BufferGPU & operator=(BufferGPU && rhs) noexcept;

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
  VkBuffer m_buffer = VK_NULL_HANDLE;

private:
  BufferGPU(const BufferGPU &) = delete;
  BufferGPU & operator=(const BufferGPU &) = delete;
};
} // namespace RHI::vulkan
