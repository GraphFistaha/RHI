#include "FramebufferAttachment.hpp"

namespace RHI::vulkan
{
VkAttachmentDescription FramebufferAttachment::BuildAttachmentDescription() const noexcept
{
  VkAttachmentDescription renderPassAttachment{};
  renderPassAttachment.format = GetImageFormat();
  renderPassAttachment.samples = GetSamplesCount(); // MSAA
  // action for color/depth buffers in pass begins
  renderPassAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  // action for color/depth buffers in pass ends
  renderPassAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  renderPassAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // action
  renderPassAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  renderPassAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  renderPassAttachment.finalLayout = GetImageLayout();
  return renderPassAttachment;
}
} // namespace RHI::vulkan
