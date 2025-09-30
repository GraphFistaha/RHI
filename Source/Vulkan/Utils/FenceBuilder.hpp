#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
struct FenceBuilder final
{
  VkFence Make(const VkDevice & device) const;
  FenceBuilder & SetLocked(bool locked = true);

private:
  bool m_locked = false;
};
} // namespace RHI::vulkan::utils
