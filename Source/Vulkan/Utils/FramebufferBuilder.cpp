#include "FramebufferBuilder.hpp"

namespace RHI::vulkan::utils //--------------- Framebuffer builder ------------
{
void FramebufferBuilder::AddAttachment(const vk::ImageView & image)
{
  m_images.push_back(image);
}

vk::ImageView & FramebufferBuilder::SetAttachment(size_t idx) & noexcept
{
  return m_images[idx];
}

vk::Framebuffer FramebufferBuilder::Make(const vk::Device & device,
                                         const vk::RenderPass & renderPass,
                                         const vk::Extent2D & extent) const
{
  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = renderPass;
  framebufferInfo.attachmentCount = static_cast<uint32_t>(m_images.size());
  framebufferInfo.pAttachments = reinterpret_cast<const VkImageView *>(m_images.data());
  framebufferInfo.width = extent.width;
  framebufferInfo.height = extent.height;
  framebufferInfo.layers = 1; // ���-�� �����������
  VkFramebuffer framebuffer;
  if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
    throw std::runtime_error("failed to create framebuffer!");
  return vk::Framebuffer(framebuffer);
}

void FramebufferBuilder::Reset()
{
  m_images.clear();
}

} // namespace RHI::vulkan::utils
