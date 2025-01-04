#pragma once
#include "../CommandsExecution/CommandBuffer.hpp"
#include "../Resources/BufferGPU.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{
struct ImageGPU : public IImageGPU,
                  private details::BufferBase
{
  explicit ImageGPU(const Context & ctx, Transferer & transferer,
                    const ImageCreateArguments & args);
  virtual ~ImageGPU() override;


  virtual void UploadImage(const uint8_t * data, const CopyImageArguments & args) override;
  virtual size_t Size() const noexcept override { return BufferBase::Size(); }

  virtual ImageExtent GetExtent() const noexcept override;
  virtual ImageType GetImageType() const noexcept override;
  virtual ImageFormat GetImageFormat() const noexcept override;

public:
  VkImage GetHandle() const noexcept;
  void SetImageLayout(details::CommandBuffer & commandBuffer, VkImageLayout newLayout) noexcept;

  const ImageCreateArguments & GetParameters() const & noexcept { return m_args; }

private:
  VkImage m_image;
  VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkFormat m_internalFormat;
  ImageCreateArguments m_args;
};

} // namespace RHI::vulkan
