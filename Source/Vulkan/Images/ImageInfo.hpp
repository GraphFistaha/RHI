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

VkImageCreateInfo BuildImageCreateInfo(const ImageCreateArguments & description) noexcept;

void TransferImageLayout(details::CommandBuffer & commandBuffer, VkImageLayout prevImageLayout,
                         VkImageLayout newLayout, VkImage image);
} // namespace RHI::vulkan
