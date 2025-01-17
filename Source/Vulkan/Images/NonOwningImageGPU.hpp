#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
struct Transferer;
} // namespace RHI::vulkan

namespace RHI::vulkan
{
    // TODO: make parent class for images - ImageBase
struct NonOwningImageGPU : public RHI::IImageGPU
{
  explicit NonOwningImageGPU(const Context & ctx, Transferer & transferer, VkImage image,
                             VkExtent3D extent, VkFormat internalFormat);
  virtual ~NonOwningImageGPU() override = default;
  NonOwningImageGPU(NonOwningImageGPU && rhs) noexcept;
  NonOwningImageGPU & operator=(NonOwningImageGPU && rhs) noexcept;


  virtual void UploadImage(const uint8_t * data, const CopyImageArguments & args) override;
  virtual size_t Size() const noexcept override;

  virtual ImageExtent GetExtent() const noexcept override;
  virtual ImageType GetImageType() const noexcept override;
  virtual ImageFormat GetImageFormat() const noexcept override;

public:
  VkImage GetHandle() const noexcept;


private:
  const Context & m_context;
  Transferer * m_transferer;

  VkImage m_image = VK_NULL_HANDLE;
  VkExtent3D m_extent;
  VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkFormat m_internalFormat;

private:
  NonOwningImageGPU(const NonOwningImageGPU &) = delete;
  NonOwningImageGPU & operator=(const NonOwningImageGPU &) = delete;
};
} // namespace RHI::vulkan
