#pragma once

#include <vulkan/vulkan.h>

namespace RHI::vulkan::details
{
/// @brief Converts one explicit core stage; rejects empty, combined, and unsupported flags.
VkPipelineStageFlags ConvertPipelineStageFromVk2ToVk1(VkPipelineStageFlagBits2 stage);

/// @brief Converts each access separately and combines the results; an empty mask stays empty.
VkAccessFlags ConvertAccessMaskFromVk2ToVk1(VkAccessFlags2 accesses);
} // namespace RHI::vulkan::details
