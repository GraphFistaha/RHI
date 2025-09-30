#include "SemaphoreBuilder.hpp"

namespace RHI::vulkan::utils
{

VkSemaphore SemaphoreBuilder::Make(const VkDevice & device) const
{
  VkSemaphore result = VK_NULL_HANDLE;
  VkSemaphoreCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  // Don't use createSemaphore in dispatchTable because it's broken
  if (vkCreateSemaphore(device, &info, nullptr, &result) != VK_SUCCESS)
    throw std::runtime_error("failed to create semaphore");
  return VkSemaphore(result);
}

} // namespace RHI::vulkan::utils
