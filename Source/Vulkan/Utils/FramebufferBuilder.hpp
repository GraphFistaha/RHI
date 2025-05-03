#pragma once

#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{

struct FramebufferBuilder final
{
  void BindAttachment(size_t idx, VkImageView imgView);
  VkFramebuffer Make(const VkDevice & device, const VkRenderPass & renderPass,
                     const VkExtent3D & extent) const;
  void Reset();

private:
  std::vector<VkImageView> m_images;
};

} // namespace RHI::vulkan::utils
