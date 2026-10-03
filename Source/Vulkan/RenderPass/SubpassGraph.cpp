#include "SubpassGraph.hpp"

#include <algorithm>
#include <numeric>
#include <ranges>
#include <span>

#include <Attachments/Attachment.hpp>
#include <RenderPass/Framebuffer.hpp>

/// @brief Compare operator for VkAttachmentDescription
static bool operator==(const VkAttachmentDescription & lhs,
                              const VkAttachmentDescription & rhs) noexcept
{
  return std::memcmp(&lhs, &rhs, sizeof(VkAttachmentDescription)) == 0;
}

namespace
{
RHI::vulkan::ResourceState CalcAttachmentBarrier(const RHI::vulkan::ResourceState & prevBarrier,
                                                 VkImageLayout newLayout) noexcept
{
  VkPipelineStageFlags2 stage = 0;
  VkAccessFlags2 access = 0;

  switch (newLayout)
  {
    // Color-output
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
      access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
      break;

    // Depth-stencil output
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
      stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
              VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
      access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
               VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      break;

    // depth-stencil input
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
      stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
              VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
      access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      break;

    // input
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
              VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
      access = VK_ACCESS_2_SHADER_READ_BIT;
      break;

    case VK_IMAGE_LAYOUT_UNDEFINED:       // Should not transition to undefined in a dependency
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: // present output
      stage = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
      access = VK_ACCESS_2_NONE; // Present doesn't write to the image
      break;

    case VK_IMAGE_LAYOUT_GENERAL:
    default:
      // Conservative fallback for unknown layouts
      // General layout could be used for many purposes, include common stages
      stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
      access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
      break;
  }

  return RHI::vulkan::ResourceState{stage, access, newLayout};
}

bool IsFramebufferSpaceStage(VkPipelineStageFlags stage) noexcept
{
  const VkPipelineStageFlags framebufferStages =
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  return static_cast<bool>(stage & framebufferStages);
}

std::array<std::span<const VkAttachmentReference>, 4> ExtractSubpassAttachments(
  const VkSubpassDescription & description) noexcept
{
  using Span = std::span<const VkAttachmentReference>;
  return {Span(description.pColorAttachments, description.colorAttachmentCount),
          description.pDepthStencilAttachment ? Span(description.pDepthStencilAttachment, 1)
                                              : Span(),
          Span(description.pInputAttachments, description.inputAttachmentCount),
          Span(description.pResolveAttachments, description.colorAttachmentCount)};
}

} // namespace

namespace RHI::vulkan
{
SubpassGraph::SubpassGraph(Framebuffer & framebuffer, size_t requiredSubpasses)
  : OwnedBy<Framebuffer>(framebuffer)
  // +2 because of the table contains info about initialRenderPass and finalRenderPass
  , m_attachmentsUsageTable(requiredSubpasses + 2, framebuffer.GetAttachments().size())
{
  auto descr = framebuffer.GetAttachementsDescription();
  m_attachmentsDescription.assign(descr.begin(), descr.end());
  auto && attachments = framebuffer.GetAttachments();

  m_subpassDescriptions.reserve(requiredSubpasses);
  auto firstRow = m_attachmentsUsageTable[GetBarrierRowIndex(SubpassIndex::initialRenderPass)];
  auto lastRow = m_attachmentsUsageTable[GetBarrierRowIndex(SubpassIndex::finalRenderPass)];

  // fill initial and final stages
  for (size_t i = 0; i < m_attachmentsDescription.size(); ++i)
  {
    // initial barrier for each attachment
    firstRow[i] = ResourceState{VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                                m_attachmentsDescription[i].initialLayout};
    // final barrier for each attachment
    lastRow[i] = ResourceState{VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                               m_attachmentsDescription[i].finalLayout};
  }

  m_prevState.assign(firstRow.begin(), firstRow.end());
}

SubpassIndex SubpassGraph::AddSubpass(const VkSubpassDescription & subpass)
{
  auto processAttachments = [this](std::span<const VkAttachmentReference> refs, SubpassIndex index)
  {
    auto row = m_attachmentsUsageTable[GetBarrierRowIndex(index)];
    for (VkAttachmentReference ref : refs)
    {
      auto * attachment = GetFramebuffer().GetAttachment(ref.attachment);
      row[ref.attachment] = CalcAttachmentBarrier(m_prevState[ref.attachment], ref.layout);
      m_prevState[ref.attachment] = row[ref.attachment];
    }
  };

  SubpassIndex index = static_cast<SubpassIndex>(m_subpassDescriptions.size());
  m_subpassDescriptions.push_back(subpass);

  { // calc row in m_attachmentsUsageTable
    auto [colorAttachments, dsAttachments, inputAttachments, resolveAttachments] =
      ExtractSubpassAttachments(subpass);
    processAttachments(inputAttachments, index);
    processAttachments(colorAttachments, index);
    processAttachments(resolveAttachments, index);
    processAttachments(dsAttachments, index);
  }

  return index;
}

void SubpassGraph::AddExternalDependency(const ResourceState & externalState, SubpassIndex subpass,
                                         uint32_t attachmentIdx)
{
  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = static_cast<uint32_t>(subpass);
  dependency.srcStageMask = externalState.currentStage;
  dependency.srcAccessMask = externalState.requiredAccess;
  auto requiredLayout =
    m_attachmentsUsageTable[GetBarrierRowIndex(subpass)][attachmentIdx].requiredLayout;
  ResourceState newBarrier = CalcAttachmentBarrier(externalState, requiredLayout);
  dependency.dstStageMask = newBarrier.currentStage;
  dependency.dstAccessMask = newBarrier.requiredAccess;
  m_dependenciesGraph.push_back(dependency);
}

void SubpassGraph::AddSelfDependency(SubpassIndex subpass)
{
    VkSubpassDependency dependency{};
    dependency.srcSubpass = static_cast<uint32_t>(subpass);
    dependency.dstSubpass = static_cast<uint32_t>(subpass);
    /*dependency.srcStageMask = externalState.currentStage;
    dependency.srcAccessMask = externalState.requiredAccess;
    auto requiredLayout =
        m_attachmentsUsageTable[GetBarrierRowIndex(subpass)][attachmentIdx].requiredLayout;
    ResourceState newBarrier = CalcAttachmentBarrier(externalState, requiredLayout);
    dependency.dstStageMask = newBarrier.currentStage;
    dependency.dstAccessMask = newBarrier.requiredAccess;
    m_dependenciesGraph.push_back(dependency);*/
}

VkRenderPass SubpassGraph::MakeRenderPass(const VkDevice & device) const
{
  if (m_subpassDescriptions.empty())
    return VK_NULL_HANDLE;

  VkRenderPass renderPass = VK_NULL_HANDLE;
  VkRenderPassCreateInfo renderPassCreateInfo{};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(m_attachmentsDescription.size());
  renderPassCreateInfo.pAttachments = m_attachmentsDescription.data();
  renderPassCreateInfo.subpassCount = static_cast<uint32_t>(m_subpassDescriptions.size());
  renderPassCreateInfo.pSubpasses = m_subpassDescriptions.data();
  renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(m_dependenciesGraph.size());
  renderPassCreateInfo.pDependencies = m_dependenciesGraph.data();

  if (auto res = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
      res != VK_SUCCESS)
    throw std::runtime_error("Failed to create render pass");

  return renderPass;
}

void SubpassGraph::SynchronizeAttachmentsDuringRenderPass(
  SubpassIndex subpassIndex, std::span<IInternalAttachment *> attachments) const
{
  auto barriersRow = m_attachmentsUsageTable[GetBarrierRowIndex(subpassIndex)];
  for (size_t i = 0; auto attachment : attachments)
  {
    if (attachment)
      attachment->GetSynchronizer().ExternalSynchronization(barriersRow[i]);
    ++i;
  }
}

size_t SubpassGraph::GetBarrierRowIndex(SubpassIndex idx) const noexcept
{
  switch (idx)
  {
    case SubpassIndex::initialRenderPass:
      return 0;
    case SubpassIndex::finalRenderPass:
      return m_subpassDescriptions.size() + 1;
    default:
      return static_cast<size_t>(idx) + 1;
  }
}

void SubpassGraph::BuildDependencyGraph(std::span<SubpassIndex> selfDependencies)
{
  size_t subpassesCount = m_subpassDescriptions.size();
  std::vector<VkSubpassDependency> dependencies;
  dependencies.reserve(subpassesCount + selfDependencies.size());

  auto prevRow = m_attachmentsUsageTable[GetBarrierRowIndex(SubpassIndex::initialRenderPass)];
  for (size_t i = 0; i < subpassesCount; ++i)
  {
    auto row = m_attachmentsUsageTable[GetBarrierRowIndex(static_cast<SubpassIndex>(i))];

    auto depInfo = dependencies.emplace_back();
    depInfo.srcSubpass = i == 0 ? VK_SUBPASS_EXTERNAL : i - 1;
    depInfo.dstSubpass = i;
    if (i == 0)
    {
      for (auto && barrier : prevRow)
      {
        depInfo.srcStageMask |= barrier.currentStage;
        depInfo.srcAccessMask |= barrier.requiredAccess;
      }
    }
    else
    {
      //TODO: find last used attachment and extrace masks from them.
      depInfo.srcStageMask = dependencies[i - 1].dstStageMask;
      depInfo.srcAccessMask = dependencies[i - 1].dstAccessMask;
    }
    for (auto && barrier : row)
    {
      depInfo.dstStageMask |= barrier.currentStage;
      depInfo.dstAccessMask |= barrier.requiredAccess;
    }
    prevRow = row;
  }

  for (SubpassIndex idx : selfDependencies)
  {
    auto && description = m_subpassDescriptions[static_cast<uint32_t>(idx)];
    auto [colorAttachments, dsAttachments, inputAttachments, resolveAttachments] =
      ExtractSubpassAttachments(description);
    VkSubpassDependency selfDependency{};
    selfDependency.srcSubpass = selfDependency.dstSubpass = static_cast<uint32_t>(idx);
    selfDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    // Handle color attachments: writes happen at COLOR_ATTACHMENT_OUTPUT,
    // reads (if any) also happen at COLOR_ATTACHMENT_OUTPUT
    if (!colorAttachments.empty())
    {
      selfDependency.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      selfDependency.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      selfDependency.dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      selfDependency.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    // Handle depth/stencil attachments: writes and reads happen at
    // EARLY_FRAGMENT_TESTS and LATE_FRAGMENT_TESTS
    if (!dsAttachments.empty())
    {
      selfDependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                     VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      selfDependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      selfDependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                     VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      selfDependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    // Handle input attachments: reads happen at FRAGMENT_SHADER stage
    if (!inputAttachments.empty())
    {
      // Input attachments are read in the fragment shader.
      // For a self-dependency, we need to synchronize the writes to those
      // attachments (which could be color/depth writes in the same subpass)
      // with the reads in the fragment shader.
      selfDependency.dstStageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      selfDependency.dstAccessMask |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

      // If there are no color or depth attachments, we still need a source
      // for the dependency. Use the same fragment shader stage as source
      // with no specific access (just execution dependency).
      if (colorAttachments.empty() && dsAttachments.empty())
      {
        selfDependency.srcStageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        // No srcAccessMask needed for pure execution dependency
      }
    }
    assert(!IsFramebufferSpaceStage(selfDependency.srcStageMask) ||
           IsFramebufferSpaceStage(selfDependency.dstStageMask));
    dependencies.push_back(selfDependency);
  }

  m_dependenciesGraph = std::move(dependencies);
}

} // namespace RHI::vulkan
