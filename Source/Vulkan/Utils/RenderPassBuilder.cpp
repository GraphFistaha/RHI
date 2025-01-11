#include "RenderPassBuilder.hpp"

namespace RHI::vulkan::utils
{

void RenderPassBuilder::AddAttachment(const VkAttachmentDescription & description)
{
  m_attachments.push_back(description);
}

void RenderPassBuilder::AddSubpass(const VkSubpassDescription & description)
{
  m_subpassDescriptions.push_back(description);
}

VkRenderPass RenderPassBuilder::Make(const VkDevice & device) const
{
  if (m_subpassDescriptions.empty() || m_attachments.empty())
    return VK_NULL_HANDLE;

  VkRenderPass renderPass = VK_NULL_HANDLE;

  std::vector<VkSubpassDependency> dependencies;

  for (uint32_t i = 0; i < m_subpassDescriptions.size(); ++i)
  {
    VkSubpassDependency dependencyInfo{};
    dependencyInfo.srcSubpass = i == 0 ? VK_SUBPASS_EXTERNAL : i - 1;
    dependencyInfo.dstSubpass = i;
    dependencyInfo.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencyInfo.srcAccessMask = 0;
    dependencyInfo.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencyInfo.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies.push_back(dependencyInfo);
  }

  VkRenderPassCreateInfo renderPassCreateInfo{};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(m_attachments.size());
  renderPassCreateInfo.pAttachments = m_attachments.data();
  renderPassCreateInfo.subpassCount = static_cast<uint32_t>(m_subpassDescriptions.size());
  renderPassCreateInfo.pSubpasses = m_subpassDescriptions.data();
  renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassCreateInfo.pDependencies = dependencies.data();


  if (auto res = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
      res != VK_SUCCESS)
    throw std::runtime_error("Failed to create render pass");

  return renderPass;
}

void RenderPassBuilder::Reset()
{
  m_attachments.clear();
  m_subpassDescriptions.clear();
  //m_usageFlags.clear();
}
} // namespace RHI::vulkan::utils
