#pragma once
#include <vector>

#include <Memory/Synchronizer.hpp>

namespace RHI::vulkan::details
{
// Only core masks are supported here. Extension scopes require explicit mappings and feature checks.
VkPipelineStageFlags ToLegacyStages(VkPipelineStageFlags2 stages);

VkAccessFlags ToLegacyAccess(VkAccessFlags2 access);

VkSubpassDependency MakeSubpassDependency(uint32_t source, uint32_t destination,
                                         const ResourceState & sourceState,
                                         const ResourceState & destinationState,
                                         bool byRegion);

std::vector<ResourceState> BuildSubpassAttachmentStates(
  const VkSubpassDescription & description, size_t attachmentCount);
} // namespace RHI::vulkan::details
