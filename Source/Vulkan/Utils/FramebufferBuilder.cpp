#include "FramebufferBuilder.hpp"

namespace RHI::vulkan::utils //--------------- Framebuffer builder ------------
{

void FramebufferBuilder::BindAttachment(size_t idx, VkImageView imgView)
{
  while (idx >= m_images.size())
    m_images.push_back(VK_NULL_HANDLE);
  m_images[idx] = imgView;
}

VkFramebuffer FramebufferBuilder::Make(const VkDevice & device, const VkRenderPass & renderPass,
                                       const VkExtent3D & extent) const
{
  if (std::any_of(m_images.begin(), m_images.end(), [](VkImageView view) { return !view; }))
    throw std::runtime_error("Some framebuffer attachments have no bound images");
  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = renderPass;
  framebufferInfo.attachmentCount = static_cast<uint32_t>(m_images.size());
  framebufferInfo.pAttachments = reinterpret_cast<const VkImageView *>(m_images.data());
  framebufferInfo.width = extent.width;
  framebufferInfo.height = extent.height;
  framebufferInfo.layers = extent.depth;
  VkFramebuffer framebuffer;
  if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
    throw std::runtime_error("failed to create framebuffer!");
  return VkFramebuffer(framebuffer);
}

void FramebufferBuilder::Reset()
{
  m_images.clear();
}

} // namespace RHI::vulkan::utils
