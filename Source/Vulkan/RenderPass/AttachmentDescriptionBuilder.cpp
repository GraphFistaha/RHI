#include "AttachmentDescriptionBuilder.hpp"

#include <Memory/Synchronizer.hpp>
#include <Utils/CastHelper.hpp>

namespace RHI::vulkan
{
VkAttachmentDescription BuildPassAttachmentDescription(IInternalAttachment & attachment,
                                                       VkImageLayout layoutAfterPass,
                                                       VkAttachmentLoadOp loadOp,
                                                       VkAttachmentStoreOp storeOp) noexcept
{
  VkAttachmentDescription attachmentDescription{};
  {
    attachmentDescription.format = attachment.GetInternalFormat();
    attachmentDescription.samples =
      RHI::vulkan::utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(
        attachment.GetSamplesCount());
    // Note: that is a promise to being attachment in that layout before render pass begins
    attachmentDescription.initialLayout = attachment.GetLayout();
    // Note:that is a promise to being attachment in that layout after render pass ends
    attachmentDescription.finalLayout = layoutAfterPass;
    attachmentDescription.loadOp = loadOp;
    attachmentDescription.storeOp = storeOp;
    attachmentDescription.stencilLoadOp = loadOp;
    attachmentDescription.stencilStoreOp = storeOp;
  }
  return attachmentDescription;
}
} // namespace RHI::vulkan
