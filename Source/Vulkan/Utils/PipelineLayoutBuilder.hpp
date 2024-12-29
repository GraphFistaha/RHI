#pragma once
#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
struct PipelineLayoutBuilder final
{
  vk::PipelineLayout Make(const vk::Device & device, const vk::DescriptorSetLayout & layout,
                          const VkPushConstantRange * pushConstantRange) const;
  void Reset() { m_layouts.clear(); }

private:
  std::vector<VkDescriptorSetLayout> m_layouts;
};
} // namespace RHI::vulkan::utils
