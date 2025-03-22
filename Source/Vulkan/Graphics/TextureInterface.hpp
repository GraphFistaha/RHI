#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::details
{
struct CommandBuffer;
}

namespace RHI::vulkan
{
static constexpr uint32_t g_InvalidImageIndex = -1;

struct ITexture : public IImageGPU
{
  virtual ~ITexture() = default;
  virtual VkImageView GetImageView(TextureUsage usage) const noexcept = 0;
  virtual void TransferLayout(details::CommandBuffer & commandBuffer, VkImageLayout layout) = 0;
  virtual VkImageLayout GetLayout() const noexcept = 0;
  virtual VkImage GetHandle() const noexcept = 0;
  virtual VkFormat GetInternalFormat() const noexcept = 0;
  virtual VkExtent3D GetInternalExtent() const noexcept = 0;
  virtual void AllowUsage(RHI::TextureUsage usage) noexcept = 0;
};

struct IAttachment : public ITexture
{
  virtual ~IAttachment() = default;
  virtual void Invalidate() = 0;
  virtual std::pair<VkImageView, VkSemaphore> AcquireForRendering() = 0;
  virtual bool FinalRendering(VkSemaphore waitSemaphore) = 0;
  virtual void SetBuffering(uint32_t framesCount) = 0;
  virtual uint32_t GetBuffering() const noexcept = 0;
  virtual VkAttachmentDescription BuildDescription() const noexcept = 0;
  virtual void TransferLayout(VkImageLayout layout) noexcept = 0;
};

} // namespace RHI::vulkan
