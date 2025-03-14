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

enum ImageUsage
{
  Transfer = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
  FramebufferAttachment = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
  Sampler = VK_IMAGE_USAGE_SAMPLED_BIT
};

struct ITexture : public IImageGPU
{
  virtual ~ITexture() = default;
  virtual VkImageView GetImageView(ImageUsage usage) const noexcept = 0;
  virtual void TransferLayout(details::CommandBuffer & commandBuffer, VkImageLayout layout) = 0;
  virtual VkImageLayout GetLayout() const noexcept = 0;
  virtual VkImage GetHandle() const noexcept = 0;
  virtual VkFormat GetInternalFormat() const noexcept = 0;
  virtual VkExtent3D GetInternalExtent() const noexcept = 0;
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
