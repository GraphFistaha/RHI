#include "FramebufferBuilder.hpp"

namespace RHI::vulkan::utils //--------------- Framebuffer builder ------------
{

VkFramebuffer FramebufferBuilder::Make(const VkDevice & device, const VkRenderPass & renderPass,
                                       const VkExtent3D & extent,
                                       std::span<const VkImageView> images) const
{
  if (std::any_of(images.begin(), images.end(), [](VkImageView view) { return !view; }))
    throw std::runtime_error("Some framebuffer attachments have no bound images");
  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = renderPass;
  framebufferInfo.attachmentCount = static_cast<uint32_t>(images.size());
  framebufferInfo.pAttachments = reinterpret_cast<const VkImageView *>(images.data());
  framebufferInfo.width = extent.width;
  framebufferInfo.height = extent.height;
  framebufferInfo.layers = extent.depth;
  VkFramebuffer framebuffer;
  if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
    throw std::runtime_error("failed to create framebuffer!");
  return VkFramebuffer(framebuffer);
}

} // namespace RHI::vulkan::utils
