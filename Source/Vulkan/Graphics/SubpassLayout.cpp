#include "SubpassLayout.hpp"

bool operator==(const VkAttachmentReference & ref1, const VkAttachmentReference & ref2) noexcept
{
  return std::memcmp(&ref1, &ref2, sizeof(VkAttachmentReference)) == 0;
}

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

void SubpassLayout::SetAttachment(ShaderImageSlot slot, uint32_t idx)
{
  auto assignOrInsert =
    [](std::vector<VkAttachmentReference> & atts, const VkAttachmentReference & ref)
  {
    auto it = std::find(atts.begin(), atts.end(), ref);
    if (it == atts.end())
      atts.push_back(ref);
    else
      *it = ref;
  };

  switch (slot)
  {
    case ShaderImageSlot::Color:
      assignOrInsert(m_colorAttachments,
                     VkAttachmentReference{idx, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
      break;
    case ShaderImageSlot::Input:
      assignOrInsert(m_inputAttachments,
                     VkAttachmentReference{idx, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL});
      break;
    case ShaderImageSlot::DepthStencil:
      m_depthStencilAttachment =
        VkAttachmentReference{idx, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL};
      break;
    default:
      throw std::invalid_argument("Unknown ShaderImageSlot");
      break;
  }
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
