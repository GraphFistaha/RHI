#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
VkAttachmentDescription BuildAttachmentDescription(
  const ImageCreateArguments & description) noexcept;

VkImageCreateInfo BuildImageCreateInfo(const ImageCreateArguments & description) noexcept;
} // namespace RHI::vulkan
