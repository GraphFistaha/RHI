#pragma once

#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{

struct FramebufferBuilder final
{
  void AddAttachment(const vk::ImageView & image);
  vk::ImageView & SetAttachment(size_t idx) & noexcept;
  vk::Framebuffer Make(const vk::Device & device, const vk::RenderPass & renderPass,
                       const vk::Extent2D & extent) const;
  void Reset();

private:
  std::vector<vk::ImageView> m_images;
};

} // namespace RHI::vulkan::utils
