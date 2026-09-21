#pragma once

#include <span>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{

struct FramebufferBuilder final
{
  VkFramebuffer Make(const VkDevice & device, const VkRenderPass & renderPass,
                     const VkExtent3D & extent, std::span<const VkImageView> attachments) const;
};

} // namespace RHI::vulkan::utils
