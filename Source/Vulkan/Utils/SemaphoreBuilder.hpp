#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
struct SemaphoreBuilder final
{
  VkSemaphore Make(const VkDevice & device) const;
};
} // namespace RHI::vulkan::utils
