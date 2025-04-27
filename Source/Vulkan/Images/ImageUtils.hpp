#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageViewType type);
}
