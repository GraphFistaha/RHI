#pragma once
#include <vector>

#include <Memory/Synchronizer.hpp>

namespace RHI::vulkan::details
{
VkSubpassDependency MakeSubpassDependency(uint32_t source, uint32_t destination,
                                         const ResourceState & sourceState,
                                         const ResourceState & destinationState,
                                         bool byRegion);

std::vector<ResourceState> BuildSubpassAttachmentStates(
  const VkSubpassDescription & description, size_t attachmentCount);
} // namespace RHI::vulkan::details
