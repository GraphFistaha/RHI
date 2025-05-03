#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct SubpassLayout final
{
  explicit SubpassLayout(VkPipelineBindPoint bindPoint);

  bool UseDepthStencil() const noexcept;
  void BindAttachment(ShaderAttachmentSlot slot, uint32_t idx);
  VkSubpassDescription BuildDescription() const noexcept;

private:
  VkPipelineBindPoint m_bindPoint;
  std::vector<VkAttachmentReference> m_colorAttachments;
  std::vector<VkAttachmentReference> m_inputAttachments;
  std::vector<uint32_t> m_preservedAttachments;
  VkAttachmentReference m_depthStencilAttachment{VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED};
};
} // namespace RHI::vulkan
