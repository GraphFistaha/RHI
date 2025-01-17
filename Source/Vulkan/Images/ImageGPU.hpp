#pragma once
#include "../BuffersAllocator.hpp"
#include "ImageBase.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{
struct ImageGPU : public IImageGPU,
                  private details::ImageBase
{
  explicit ImageGPU(const Context & ctx, Transferer & transferer,
                    const ImageCreateArguments & args);
  virtual ~ImageGPU() override;
  ImageGPU(ImageGPU && rhs) noexcept;
  ImageGPU & operator=(ImageGPU && rhs) noexcept;

  virtual void UploadImage(const uint8_t * data, const CopyImageArguments & args) override;
  virtual size_t Size() const noexcept override { return ImageBase::Size(); }

  virtual ImageExtent GetExtent() const noexcept override;
  virtual ImageType GetImageType() const noexcept override;
  virtual ImageFormat GetImageFormat() const noexcept override;


  const ImageCreateArguments & GetParameters() const & noexcept { return m_args; }

public:
  void SetImageLayout(details::CommandBuffer & commandBuffer, VkImageLayout newLayout) noexcept;

private:
  BuffersAllocator::AllocInfoRawMemory m_allocInfo;
  InternalObjectHandle m_memBlock = nullptr;
  uint32_t m_flags = 0;
  size_t m_size = 0;
  ImageCreateArguments m_args;
};

} // namespace RHI::vulkan
