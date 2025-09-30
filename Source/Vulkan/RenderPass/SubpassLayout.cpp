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

void SubpassLayout::BindAttachment(ShaderAttachmentSlot slot, uint32_t idx)
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

  if (slot & ShaderAttachmentSlot::Preserved)
  {
    m_preservedAttachments.push_back(idx);
    return;
  }

  switch (slot)
  {
    case ShaderAttachmentSlot::Color:
      assignOrInsert(m_colorAttachments,
                     VkAttachmentReference{idx, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
      assignOrInsert(m_resolveAttachments,
                     VkAttachmentReference{VK_ATTACHMENT_UNUSED,
                                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
      break;
    case ShaderAttachmentSlot::Input:
      assignOrInsert(m_inputAttachments,
                     VkAttachmentReference{idx, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL});
      break;
    case ShaderAttachmentSlot::DepthStencil:
      m_depthStencilAttachment =
        VkAttachmentReference{idx, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
      break;
    default:
      throw std::invalid_argument("Unknown ShaderAttachmentSlot");
      break;
  }
}

void SubpassLayout::BindResolver(uint32_t idx, uint32_t resolve_idx)
{
  m_resolveAttachments[resolve_idx].attachment = idx;
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
  // here must be attachments that should be MSAA resolved
  subpassDescription.pResolveAttachments = m_resolveAttachments.data();
  subpassDescription.pPreserveAttachments = m_preservedAttachments.data();
  subpassDescription.preserveAttachmentCount = static_cast<uint32_t>(m_preservedAttachments.size());
  return subpassDescription;
}

void SubpassLayout::ForEachAttachment(std::function<void(uint32_t attachmentIdx)> && func) const
{
  std::for_each(m_colorAttachments.begin(), m_colorAttachments.end(),
                [&func](const VkAttachmentReference & ref) { func(ref.attachment); });
  std::for_each(m_inputAttachments.begin(), m_inputAttachments.end(),
                [&func](const VkAttachmentReference & ref) { func(ref.attachment); });
  if (UseDepthStencil())
    func(m_depthStencilAttachment.attachment);
}

} // namespace RHI::vulkan
