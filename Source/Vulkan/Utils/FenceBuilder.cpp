#include "FenceBuilder.hpp"

namespace RHI::vulkan::utils
{

VkFence FenceBuilder::Make(const VkDevice & device) const
{
  VkFenceCreateInfo info{};
  VkFence result = VK_NULL_HANDLE;
  info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (m_locked)
    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  // Don't use createFence in dispatchTable because it's broken
  if (vkCreateFence(device, &info, nullptr, &result) != VK_SUCCESS)
    throw std::runtime_error("failed to create fence");
  return result;
}


FenceBuilder & FenceBuilder::SetLocked(bool locked)
{
  m_locked = locked;
  return *this;
}
} // namespace RHI::vulkan::utils
