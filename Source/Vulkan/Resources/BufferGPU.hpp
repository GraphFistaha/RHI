#pragma once
#include <OwnedBy.hpp>
#include <RHI.hpp>

#include "../Memory/MemoryBlock.hpp"

namespace RHI::vulkan
{
struct Context;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct BufferGPU : public IBufferGPU,
                   public OwnedBy<Context>
{
  using IBufferGPU::ScopedPointer;

  explicit BufferGPU(Context & ctx, size_t size, VkBufferUsageFlags usage, bool mapped = false);
  virtual ~BufferGPU() override;

  BufferGPU(BufferGPU && rhs) noexcept;
  BufferGPU & operator=(BufferGPU && rhs) noexcept;

  virtual void UploadSync(const void * data, size_t size, size_t offset = 0) override;
  virtual std::future<UploadResult> UploadAsync(const void * data, size_t size,
                                                size_t offset = 0) override;
  virtual ScopedPointer Map() override;
  virtual void Flush() const noexcept override;
  virtual bool IsMapped() const noexcept override;
  virtual size_t Size() const noexcept override;

public:
  VkBuffer GetHandle() const noexcept;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

private:
  memory::MemoryBlock m_memBlock;

private:
  BufferGPU(const BufferGPU &) = delete;
  BufferGPU & operator=(const BufferGPU &) = delete;
};
} // namespace RHI::vulkan
