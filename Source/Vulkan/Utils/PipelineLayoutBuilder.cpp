#include "PipelineLayoutBuilder.hpp"

namespace RHI::vulkan::utils
{
VkPipelineLayout PipelineLayoutBuilder::Make(const VkDevice & device,
                                               const VkDescriptorSetLayout & descriptorsLayout,
                                               const VkPushConstantRange * pushConstantRange) const
{
  auto tmp = static_cast<VkDescriptorSetLayout>(descriptorsLayout);
  // create pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = tmp == VK_NULL_HANDLE ? 0 : 1;                // Optional
  pipelineLayoutInfo.pSetLayouts = tmp == VK_NULL_HANDLE ? nullptr : &tmp;          // Optional
  pipelineLayoutInfo.pushConstantRangeCount = pushConstantRange == nullptr ? 0 : 1; // Optional
  pipelineLayoutInfo.pPushConstantRanges = pushConstantRange;                       // Optional

  VkPipelineLayout layout;
  if (auto res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &layout);
      res != VK_SUCCESS)
    throw std::runtime_error("Failed to create pipeline layout - ");
  return layout;
}
} // namespace RHI::vulkan::utils
