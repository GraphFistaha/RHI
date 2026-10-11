#pragma once
#include <Attachments/Attachment.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
/// @brief builds attachment description for render pass
/// @param attachment - attachment object
/// @param layoutAfterPass - the promise, in what layout the image will be after the pass is done
/// @param loadOp - what should you do with image content before pass (nothing, clear)
/// @param storeOp - what should you do with image content after pass (nothing, save)
VkAttachmentDescription BuildPassAttachmentDescription(
  RHI::vulkan::IInternalAttachment & attachment, VkImageLayout layoutAfterPass,
  VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp) noexcept;
} // namespace RHI::vulkan
