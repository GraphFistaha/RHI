#include "SubpassSynchronization.hpp"

#include <span>
#include <stdexcept>

namespace RHI::vulkan::details
{
// Only core masks are supported here. Extension scopes require explicit mappings and feature checks.
VkPipelineStageFlags ToLegacyStages(VkPipelineStageFlags2 stages)
{
  constexpr VkPipelineStageFlags2 legacy =
    VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT |
    VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
    VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT |
    VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT |
    VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT |
    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
    VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT | VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT |
    VK_PIPELINE_STAGE_2_HOST_BIT | VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT |
    VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  constexpr VkPipelineStageFlags2 transfer =
    VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_RESOLVE_BIT |
    VK_PIPELINE_STAGE_2_CLEAR_BIT;
  constexpr VkPipelineStageFlags2 input = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT |
                                          VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
  constexpr VkPipelineStageFlags2 preRaster = VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT;
  if (stages & ~(legacy | transfer | input | preRaster))
    throw std::invalid_argument("Unsupported synchronization2 stage mask for legacy render pass");
  auto result = static_cast<VkPipelineStageFlags>(stages & legacy);
  if (stages & transfer)
    result |= VK_PIPELINE_STAGE_TRANSFER_BIT;
  if (stages & input)
    result |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
  // ALL_GRAPHICS avoids spelling out optional geometry/tessellation stages on devices without them.
  if (stages & preRaster)
    result |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
  return result;
}

VkAccessFlags ToLegacyAccess(VkAccessFlags2 access)
{
  constexpr VkAccessFlags2 legacy =
    VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_INDEX_READ_BIT |
    VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT |
    VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT |
    VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
    VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_TRANSFER_READ_BIT |
    VK_ACCESS_2_TRANSFER_WRITE_BIT | VK_ACCESS_2_HOST_READ_BIT | VK_ACCESS_2_HOST_WRITE_BIT |
    VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
  constexpr VkAccessFlags2 shaderRead = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT |
                                        VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
  constexpr VkAccessFlags2 shaderWrite = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
  if (access & ~(legacy | shaderRead | shaderWrite))
    throw std::invalid_argument("Unsupported synchronization2 access mask for legacy render pass");
  auto result = static_cast<VkAccessFlags>(access & legacy);
  if (access & shaderRead)
    result |= VK_ACCESS_SHADER_READ_BIT;
  if (access & shaderWrite)
    result |= VK_ACCESS_SHADER_WRITE_BIT;
  return result;
}

VkSubpassDependency MakeSubpassDependency(uint32_t source, uint32_t destination,
                                                 const ResourceState & sourceState,
                                                 const ResourceState & destinationState,
                                                 bool byRegion)
{
  VkSubpassDependency result{};
  result.srcSubpass = source;
  result.dstSubpass = destination;
  result.srcStageMask = ToLegacyStages(sourceState.currentStage);
  result.dstStageMask = ToLegacyStages(destinationState.currentStage);
  result.srcAccessMask = ToLegacyAccess(sourceState.requiredAccess);
  result.dstAccessMask = ToLegacyAccess(destinationState.requiredAccess);
  // NONE scopes need equivalent legacy execution endpoints, with no accesses.
  if (!result.srcStageMask)
  {
    if (result.srcAccessMask)
      throw std::invalid_argument("A source access scope requires a stage");
    result.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  }
  if (!result.dstStageMask)
  {
    if (result.dstAccessMask)
      throw std::invalid_argument("A destination access scope requires a stage");
    result.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  }
  constexpr VkPipelineStageFlags framebuffer =
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  constexpr VkPipelineStageFlags graphics =
    framebuffer | VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
    VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
    VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  if ((source != VK_SUBPASS_EXTERNAL && (result.srcStageMask & ~graphics)) ||
      (destination != VK_SUBPASS_EXTERNAL && (result.dstStageMask & ~graphics)))
    throw std::invalid_argument("Internal subpass scopes must use graphics pipeline stages");
  if (source == destination)
  {
    // ALL_GRAPHICS/ALL_COMMANDS include framebuffer stages after expansion.
    const bool srcFramebuffer =
      (result.srcStageMask & (framebuffer | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)) != 0;
    if (srcFramebuffer && (result.dstStageMask & ~framebuffer))
      throw std::invalid_argument(
        "Framebuffer self-dependency destinations must contain only framebuffer stages");
    byRegion = srcFramebuffer;
  }
  result.dependencyFlags = byRegion ? VK_DEPENDENCY_BY_REGION_BIT : 0;
  return result;
}

std::vector<ResourceState> BuildSubpassAttachmentStates(
  const VkSubpassDescription & description, size_t attachmentCount)
{
  std::vector<ResourceState> states(attachmentCount);
  auto add =
    [&](const VkAttachmentReference & ref, VkPipelineStageFlags2 stages, VkAccessFlags2 access)
  {
    if (ref.attachment == VK_ATTACHMENT_UNUSED)
      return;
    if (ref.attachment >= attachmentCount)
      throw std::out_of_range("Subpass references an unregistered attachment");
    if (ref.layout == VK_IMAGE_LAYOUT_UNDEFINED || ref.layout == VK_IMAGE_LAYOUT_PREINITIALIZED ||
        ref.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
      throw std::invalid_argument("Invalid subpass attachment layout");
    auto & state = states[ref.attachment];
    if (state.requiredLayout != VK_IMAGE_LAYOUT_UNDEFINED && state.requiredLayout != ref.layout)
      throw std::invalid_argument("Attachment has conflicting layouts within a subpass");
    state.currentStage |= stages;
    state.requiredAccess |= access;
    state.requiredLayout = ref.layout;
  };
  if ((description.colorAttachmentCount && !description.pColorAttachments) ||
      (description.inputAttachmentCount && !description.pInputAttachments) ||
      (description.preserveAttachmentCount && !description.pPreserveAttachments))
    throw std::invalid_argument("Missing subpass attachment array");
  for (const auto & ref :
       std::span(description.pColorAttachments, description.colorAttachmentCount))
    add(ref, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
  if (description.pResolveAttachments)
    for (const auto & ref :
         std::span(description.pResolveAttachments, description.colorAttachmentCount))
      add(ref, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
          VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
  for (const auto & ref :
       std::span(description.pInputAttachments, description.inputAttachmentCount))
    add(ref, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT);
  if (description.pDepthStencilAttachment)
  {
    const auto & ref = *description.pDepthStencilAttachment;
    VkAccessFlags2 access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    if (ref.layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL &&
        ref.layout != VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL &&
        ref.layout != VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL &&
        ref.layout != VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL)
      access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    add(ref,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        access);
  }
  for (const auto index :
       std::span(description.pPreserveAttachments, description.preserveAttachmentCount))
  {
    if (index >= attachmentCount)
      throw std::out_of_range("Invalid preserve attachment index");
    if (states[index].requiredLayout != VK_IMAGE_LAYOUT_UNDEFINED)
      throw std::invalid_argument("A preserved attachment must not also be used by the subpass");
  }
  return states;
}
} // namespace RHI::vulkan::details
