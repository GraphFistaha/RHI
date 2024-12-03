#include "PipelineLayoutBuilder.hpp"

namespace RHI::vulkan::utils
{
vk::PipelineLayout PipelineLayoutBuilder::Make(
  const vk::Device & device, const vk::DescriptorSetLayout & descriptorsLayout) const
{
  auto tmp = static_cast<VkDescriptorSetLayout>(descriptorsLayout);
  // create pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = tmp == VK_NULL_HANDLE ? 0 : 1;       // Optional
  pipelineLayoutInfo.pSetLayouts = tmp == VK_NULL_HANDLE ? nullptr : &tmp; // Optional
  pipelineLayoutInfo.pushConstantRangeCount = 0;                           // Optional
  pipelineLayoutInfo.pPushConstantRanges = nullptr;                        // Optional

  VkPipelineLayout layout;
  if (auto res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &layout);
      res != VK_SUCCESS)
    throw std::runtime_error("Failed to create pipeline layout - ");
  return layout;
}
} // namespace RHI::vulkan::utils
