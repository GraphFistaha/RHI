#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Images/TextureInterface.hpp"

namespace RHI::vulkan
{
static constexpr uint32_t g_InvalidImageIndex = -1;

struct IInternalAttachment : public IInternalTexture
{
  virtual ~IInternalAttachment() = default;
  virtual void Invalidate() = 0;
  virtual std::pair<VkImageView, VkSemaphore> AcquireForRendering() = 0;
  virtual bool FinalRendering(VkSemaphore waitSemaphore) = 0;
  virtual uint32_t GetBuffering() const noexcept = 0;
  virtual RHI::SamplesCount GetSamplesCount() const noexcept = 0;
  // Rename to AddAttachmentDescriptionTo
  virtual VkAttachmentDescription BuildDescription() const noexcept = 0;
  virtual void TransferLayout(VkImageLayout layout) noexcept = 0;
  virtual void Resize(const VkExtent2D & new_extent) noexcept = 0;
};

} // namespace RHI::vulkan
