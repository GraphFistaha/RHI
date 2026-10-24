#include "PipelineAttachmentsUsage.hpp"

#include <Memory/ResourceUser.hpp>


static bool operator==(const VkAttachmentReference & ref1,
                       const VkAttachmentReference & ref2) noexcept
{
  return std::memcmp(&ref1, &ref2, sizeof(VkAttachmentReference)) == 0;
}

namespace RHI::vulkan
{
bool PipelineAttachmentsUsage::UseDepthStencil() const noexcept
{
  return m_depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED;
}

void PipelineAttachmentsUsage::BindAttachment(ShaderAttachmentSlot slot, uint32_t idx)
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
  if (slot & ShaderAttachmentSlot::Color)
  {
    assignOrInsert(m_colorAttachments,
                   VkAttachmentReference{idx, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    assignOrInsert(m_resolveAttachments,
                   VkAttachmentReference{VK_ATTACHMENT_UNUSED,
                                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
  }
  if (slot & ShaderAttachmentSlot::DepthStencil)
  {
    m_depthStencilAttachment =
      VkAttachmentReference{idx, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
  }
  if (slot & ShaderAttachmentSlot::Input)
  {
    assignOrInsert(m_inputAttachments,
                   VkAttachmentReference{idx, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL});
  }
}

void PipelineAttachmentsUsage::BindResolver(uint32_t idx, uint32_t resolve_idx)
{
  m_resolveAttachments[resolve_idx].attachment = idx;
}

VkSubpassDescription PipelineAttachmentsUsage::BuildDescription(
  VkPipelineBindPoint bindPoint) const noexcept
{
  VkSubpassDescription subpassDescription{};
  subpassDescription.pipelineBindPoint = bindPoint;
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

void PipelineAttachmentsUsage::CollectAttachmentsUsageInfo(
  std::span<VkImageUsageFlags> result) const
{
  for (auto && ref : m_colorAttachments)
  {
    result[ref.attachment] |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  }
  for (auto && ref : m_inputAttachments)
  {
    result[ref.attachment] |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
  }
  if (UseDepthStencil())
    result[m_depthStencilAttachment.attachment] |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
}

void PipelineAttachmentsUsage::CollectBarriers(std::vector<Barrier> & barriers) const
{
  /// @brief Appends synchronization requirements for active attachment references.
  const auto collect =
    [&barriers](const auto & attachments, VkPipelineStageFlags2 stages, VkAccessFlagBits2 access)
  {
    for (const auto & ref : attachments)
    {
      if (ref.attachment != VK_ATTACHMENT_UNUSED)
        barriers.push_back({ref.attachment, ResourceState{stages, access, ref.layout}});
    }
  };

  const auto colorAccess = static_cast<VkAccessFlagBits2>(VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
                                                          VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
  collect(m_colorAttachments, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, colorAccess);
  collect(m_resolveAttachments, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
          VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
  collect(m_inputAttachments, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
          VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT);
  if (UseDepthStencil())
  {
    barriers.push_back(
      {m_depthStencilAttachment.attachment,
       ResourceState{VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                       VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                     static_cast<VkAccessFlagBits2>(VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                                    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT),
                     m_depthStencilAttachment.layout}});
  }
}

} // namespace RHI::vulkan
