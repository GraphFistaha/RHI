#pragma once

#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{

struct FramebufferBuilder final
{
  void AddAttachment(const VkImageView & image);
  VkImageView & SetAttachment(size_t idx) & noexcept;
  VkFramebuffer Make(const VkDevice & device, const VkRenderPass & renderPass,
                       const VkExtent2D & extent) const;
  void Reset();

private:
  std::vector<VkImageView> m_images;
};

} // namespace RHI::vulkan::utils
