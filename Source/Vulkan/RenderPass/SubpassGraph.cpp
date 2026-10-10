#include "SubpassGraph.hpp"

#include <stdexcept>

#include <Attachments/Attachment.hpp>
#include <Pipeline/Pipeline.hpp>
#include <RenderPass/SubpassSynchronization.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{
SubpassGraph::SubpassGraph(Context & ctx)
  : OwnedBy<Context>(ctx)
{
}

SubpassGraph::~SubpassGraph()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_renderPass, nullptr);
}

void SubpassGraph::RequireMutable() const
{
  if (m_renderPass != VK_NULL_HANDLE)
    throw std::logic_error("A compiled SubpassGraph cannot be modified or compiled again");
}

size_t SubpassGraph::GetSubpassIndex(SubpassIndex index) const
{
  const auto value = static_cast<int32_t>(index);
  if (value < 0 || static_cast<size_t>(value) >= m_subpassDescriptions.size())
    throw std::out_of_range("Invalid subpass index");
  return static_cast<size_t>(value);
}

uint32_t SubpassGraph::AddAttachment(const IInternalAttachment & attachment,
                                     const ResourceState & initState,
                                     const ResourceState & finalState, VkAttachmentLoadOp loadOp,
                                     VkAttachmentStoreOp storeOp)
{
  RequireMutable();
  if (m_attachments.size() >= VK_ATTACHMENT_UNUSED)
    throw std::length_error("Too many render pass attachments");
  VkAttachmentDescription description{};
  {
    description.format = attachment.GetInternalFormat();
    description.samples =
      utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(attachment.GetSamplesCount());
    description.loadOp = description.stencilLoadOp = loadOp;
    description.storeOp = description.stencilStoreOp = storeOp;
    description.initialLayout = initState.requiredLayout;
    description.finalLayout = finalState.requiredLayout;
  }
  const auto index = static_cast<uint32_t>(m_attachments.size());

  // Reserve parallel arrays first so allocation failures cannot leave their sizes inconsistent.
  m_attachments.reserve(index + 1);
  m_initialStates.reserve(index + 1);
  m_finalStates.reserve(index + 1);
  for (auto & row : m_attachmentsUsage)
    row.reserve(index + 1);
  m_attachments.push_back(description);
  m_initialStates.push_back(initState);
  m_finalStates.push_back(finalState);
  for (auto & row : m_attachmentsUsage)
    row.emplace_back();
  return index;
}

SubpassIndex SubpassGraph::AddSubpass(const Pipeline & pipeline)
{
  RequireMutable();
  if (&pipeline.GetContext() != &GetContext())
    throw std::invalid_argument("Pipeline and graph must belong to the same context");
  if (m_subpassDescriptions.size() >= static_cast<size_t>(SubpassIndex::finalRenderPass))
    throw std::length_error("Too many subpasses");
  const auto description = pipeline.GetAttachmentUsageInfo().BuildDescription(m_bindPoint);
  auto usage = details::BuildSubpassAttachmentStates(description, m_attachments.size());
  const auto index = static_cast<SubpassIndex>(m_subpassDescriptions.size());
  m_subpassDescriptions.reserve(m_subpassDescriptions.size() + 1);
  m_attachmentsUsage.reserve(m_attachmentsUsage.size() + 1);
  m_subpassDescriptions.push_back(description);
  m_attachmentsUsage.push_back(std::move(usage));
  return index;
}

const ResourceState & SubpassGraph::GetAttachmentState(SubpassIndex subpass,
                                                       uint32_t attachment) const &
{
  const auto index = GetSubpassIndex(subpass);
  if (attachment >= m_attachments.size())
    throw std::out_of_range("Invalid attachment index");
  const auto & state = m_attachmentsUsage[index][attachment];
  if (state.requiredLayout == VK_IMAGE_LAYOUT_UNDEFINED)
    throw std::invalid_argument("Dependency attachment is not used by this subpass");
  return state;
}

void SubpassGraph::AppendDependency(uint32_t source, uint32_t destination,
                                    const ResourceState & sourceState,
                                    const ResourceState & destinationState, bool byRegion)
{
  m_dependenciesGraph.push_back(
    details::MakeSubpassDependency(source, destination, sourceState, destinationState, byRegion));
}

void SubpassGraph::AddDependency(SubpassIndex source, SubpassIndex destination, uint32_t attachment)
{
  RequireMutable();
  const auto & srcState = GetAttachmentState(source, attachment);
  const auto & dstState = GetAttachmentState(destination, attachment);
  if (static_cast<int32_t>(source) >= static_cast<int32_t>(destination))
    throw std::invalid_argument(
      "Internal dependencies must connect distinct forward-ordered subpasses");
  AppendDependency(static_cast<uint32_t>(source), static_cast<uint32_t>(destination), srcState,
                   dstState, true);
}

void SubpassGraph::AddIncomingDependency(const ResourceState & externalState,
                                         SubpassIndex destination, uint32_t attachment)
{
  RequireMutable();
  const auto & dstState = GetAttachmentState(destination, attachment);
  if (m_initialStates[attachment].requiredLayout != VK_IMAGE_LAYOUT_UNDEFINED &&
      externalState.requiredLayout != m_initialStates[attachment].requiredLayout)
    throw std::invalid_argument(
      "Incoming dependency layout must match the initial attachment layout");
  AppendDependency(VK_SUBPASS_EXTERNAL, static_cast<uint32_t>(destination), externalState, dstState,
                   false);
}

void SubpassGraph::AddOutgoingDependency(SubpassIndex source, const ResourceState & externalState,
                                         uint32_t attachment)
{
  RequireMutable();
  const auto & srcState = GetAttachmentState(source, attachment);
  if (externalState.requiredLayout != m_finalStates[attachment].requiredLayout)
    throw std::invalid_argument(
      "Outgoing dependency layout must match the final attachment layout");
  AppendDependency(static_cast<uint32_t>(source), VK_SUBPASS_EXTERNAL, srcState, externalState,
                   false);
}

void SubpassGraph::AddSelfDependency(SubpassIndex subpass, const ResourceState & sourceState,
                                     const ResourceState & destinationState)
{
  RequireMutable();
  const auto index = static_cast<uint32_t>(GetSubpassIndex(subpass));
  AppendDependency(index, index, sourceState, destinationState, false);
}

bool SubpassGraph::Compile()
{
  RequireMutable();
  if (m_subpassDescriptions.empty())
    throw std::invalid_argument("A render pass requires at least one subpass");
  for (const auto & attachment : m_attachments)
  {
    if (attachment.finalLayout == VK_IMAGE_LAYOUT_UNDEFINED ||
        attachment.finalLayout == VK_IMAGE_LAYOUT_PREINITIALIZED)
      throw std::invalid_argument("Invalid final attachment layout");
    if (attachment.initialLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        (attachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD ||
         attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD))
      throw std::invalid_argument("LOAD requires defined initial attachment contents");
  }
  VkRenderPassCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = static_cast<uint32_t>(m_attachments.size());
  info.pAttachments = m_attachments.data();
  info.subpassCount = static_cast<uint32_t>(m_subpassDescriptions.size());
  info.pSubpasses = m_subpassDescriptions.data();
  info.dependencyCount = static_cast<uint32_t>(m_dependenciesGraph.size());
  info.pDependencies = m_dependenciesGraph.data();
  VkRenderPass renderPass = VK_NULL_HANDLE;
  const auto result =
    vkCreateRenderPass(GetContext().GetGpuConnection().GetDevice(), &info, nullptr, &renderPass);
  if (result != VK_SUCCESS)
  {
    GetContext().Log(LogMessageStatus::LOG_ERROR, "Failed to create render pass: {}",
                     static_cast<int>(result));
    return false;
  }
  m_renderPass = renderPass;
  return true;
}

void SubpassGraph::SynchronizeAttachmentsDuringRenderPass(
  SubpassIndex subpassIndex, std::span<IInternalAttachment *> attachments) const
{
  if (!m_renderPass)
    throw std::logic_error("Attachment state tracking requires a compiled graph");
  if (attachments.size() != m_attachments.size())
    throw std::invalid_argument("Attachment count must match the graph");
  std::span<const ResourceState> states;
  if (subpassIndex == SubpassIndex::initialRenderPass)
    states = m_initialStates;
  else if (subpassIndex == SubpassIndex::finalRenderPass)
    states = m_finalStates;
  else
    states = m_attachmentsUsage[GetSubpassIndex(subpassIndex)];
  for (size_t i = 0; i < attachments.size(); ++i)
  {
    const auto & state = states[i];
    if (attachments[i] && (subpassIndex == SubpassIndex::initialRenderPass ||
                           subpassIndex == SubpassIndex::finalRenderPass ||
                           state.requiredLayout != VK_IMAGE_LAYOUT_UNDEFINED))
      attachments[i]->GetSynchronizer().ExternalSynchronization(state);
  }
}
} // namespace RHI::vulkan
