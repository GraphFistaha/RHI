#include "PipelineLayoutBuilder.hpp"

namespace RHI::vulkan::utils
{
VkPipelineLayout PipelineLayoutBuilder::Make(const VkDevice & device,
                                             const VkDescriptorSetLayout * layouts,
                                             uint32_t layoutSize,
                                             const VkPushConstantRange * pushConstantRange) const
{
  // create pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = layoutSize;                                   // Optional
  pipelineLayoutInfo.pSetLayouts = layouts;                                         // Optional
  pipelineLayoutInfo.pushConstantRangeCount = pushConstantRange == nullptr ? 0 : 1; // Optional
  pipelineLayoutInfo.pPushConstantRanges = pushConstantRange;                       // Optional

  VkPipelineLayout layout;
  if (auto res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &layout);
      res != VK_SUCCESS)
    throw std::runtime_error("Failed to create pipeline layout - ");
  return layout;
}
} // namespace RHI::vulkan::utils
