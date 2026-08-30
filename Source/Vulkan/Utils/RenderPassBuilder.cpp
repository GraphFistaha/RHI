#include "RenderPassBuilder.hpp"

namespace
{
std::pair<VkPipelineStageFlags, VkAccessFlags> CalcSourceStage(
  const VkAttachmentReference & ref, std::span<const VkAttachmentDescription> attachments) noexcept
{
  // layout means how the attachment is using during subpass
  switch (ref.layout)
  {
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      return {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE};
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
      return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
              VK_ACCESS_NONE};
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      return {VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_ACCESS_NONE};
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      return {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT};
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      return {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT};
    case VK_IMAGE_LAYOUT_GENERAL:
      return {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_NONE};
    default:
      // For VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PREINITIALIZED, etc.
      return {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE};
  }
}

std::pair<VkPipelineStageFlags, VkAccessFlags> CalcDestinationStage(
  const VkAttachmentReference & ref, std::span<const VkAttachmentDescription> attachments) noexcept
{
  // layout means how the attachment is using during subpass
  switch (ref.layout)
  {
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      return {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
              VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
      return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
      return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT};
    case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
      return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
    case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
      return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT};
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
      return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT};
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
      return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT};
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
      return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT};
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      return {VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
              VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT};
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      return {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT};
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      return {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT};
    case VK_IMAGE_LAYOUT_GENERAL:
      return {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
              VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT};
    default:
      // For VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PREINITIALIZED, etc.
      return {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE};
  }
}

void ProcessAttachments(std::span<const VkAttachmentReference> references,
                        std::span<const VkAttachmentDescription> attachments,
                        VkSubpassDependency & result)
{
  for (auto && ref : references)
  {
    auto [srcStage, srcAccess] = CalcSourceStage(ref, attachments);
    auto [dstStage, dstAccess] = CalcDestinationStage(ref, attachments);
    result.srcStageMask |= srcStage;
    result.srcAccessMask |= srcAccess;
    result.dstStageMask |= dstStage;
    result.dstAccessMask |= dstAccess;
  }
}

std::vector<VkSubpassDependency> BuildDependencies(
  std::span<const VkSubpassDescription> descriptions,
  std::span<const VkAttachmentDescription> attachments)
{
  std::vector<VkSubpassDependency> dependencies;

  for (uint32_t i = 0; i < descriptions.size(); ++i)
  {
    auto && description = descriptions[i];
    VkSubpassDependency dependencyInfo{};
    dependencyInfo.srcSubpass = i == 0 ? VK_SUBPASS_EXTERNAL : i - 1;
    dependencyInfo.dstSubpass = i;
    ProcessAttachments(std::span<const VkAttachmentReference>(description.pColorAttachments,
                                                              description.colorAttachmentCount),
                       attachments, dependencyInfo);
    ProcessAttachments(std::span<const VkAttachmentReference>(description.pDepthStencilAttachment,
                                                              description.pDepthStencilAttachment
                                                                ? 1
                                                                : 0),
                       attachments, dependencyInfo);
    ProcessAttachments(std::span<const VkAttachmentReference>(description.pInputAttachments,
                                                              description.inputAttachmentCount),
                       attachments, dependencyInfo);
    //ProcessAttachments(std::span<const VkAttachmentReference>(description.pPreserveAttachments,
    //                                                          description.preserveAttachmentCount),
    //                   attachments, dependencyInfo);

    dependencies.push_back(dependencyInfo);
  }
  return dependencies;
}

} // namespace

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
  std::vector<VkSubpassDependency> dependencies =
    ::BuildDependencies(m_subpassDescriptions, m_attachments);

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
