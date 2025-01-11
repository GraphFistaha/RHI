#include "SubpassLayout.hpp"

namespace RHI::vulkan
{
SubpassLayout::SubpassLayout(VkPipelineBindPoint bindPoint)
  : m_bindPoint(bindPoint)
{
}

bool SubpassLayout::UseDepthStencil() const noexcept
{
  return m_depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED;
}

void SubpassLayout::AddAttachment(ShaderImageSlot slot, uint32_t idx)
{
  switch (slot)
  {
    case ShaderImageSlot::Color:
      assert(!std::any_of(m_colorAttachments.begin(), m_colorAttachments.end(),
                          [idx](VkAttachmentReference & attachment)
                          { return attachment.attachment == idx; }));

      m_colorAttachments.push_back(
        VkAttachmentReference{idx, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
      break;
    case ShaderImageSlot::Input:
      assert(!std::any_of(m_inputAttachments.begin(), m_inputAttachments.end(),
                          [idx](VkAttachmentReference & attachment)
                          { return attachment.attachment == idx; }));
      m_inputAttachments.push_back(
        VkAttachmentReference{idx, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL});
      break;
    case ShaderImageSlot::DepthStencil:
      assert(!UseDepthStencil());
      m_depthStencilAttachment =
        VkAttachmentReference{idx, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL};
      break;
    default:
      throw std::invalid_argument("Unknown ShaderImageSlot");
      break;
  }
}

void SubpassLayout::ResetAttachments()
{
  m_colorAttachments.clear();
  m_inputAttachments.clear();
  m_depthStencilAttachment = VkAttachmentReference{VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED};
}

VkSubpassDescription SubpassLayout::BuildDescription() const noexcept
{
  VkSubpassDescription subpassDescription{};
  subpassDescription.pipelineBindPoint = m_bindPoint;
  subpassDescription.colorAttachmentCount = static_cast<uint32_t>(m_colorAttachments.size());
  subpassDescription.pColorAttachments = m_colorAttachments.data();
  subpassDescription.inputAttachmentCount = static_cast<uint32_t>(m_inputAttachments.size());
  subpassDescription.pInputAttachments = m_inputAttachments.data();
  subpassDescription.pDepthStencilAttachment = UseDepthStencil() ? &m_depthStencilAttachment
                                                                 : nullptr;
  return subpassDescription;
}

} // namespace RHI::vulkan
