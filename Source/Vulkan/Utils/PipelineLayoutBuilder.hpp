#pragma once
#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
struct PipelineLayoutBuilder final
{
  VkPipelineLayout Make(const VkDevice & device, const VkDescriptorSetLayout * layouts,
                        uint32_t layoutSize, const VkPushConstantRange * pushConstantRange) const;
  void Reset() { m_layouts.clear(); }

private:
  std::vector<VkDescriptorSetLayout> m_layouts;
};
} // namespace RHI::vulkan::utils
