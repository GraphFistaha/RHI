#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
namespace details
{
struct CommandBuffer;
}

VkAttachmentDescription BuildAttachmentDescription(
  const ImageCreateArguments & description) noexcept;

} // namespace RHI::vulkan
