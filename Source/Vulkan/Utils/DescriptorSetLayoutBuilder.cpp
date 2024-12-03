#include "DescriptorSetLayoutBuilder.hpp"

#include "CastHelper.hpp"

namespace RHI::vulkan::utils
{
vk::DescriptorSetLayout DescriptorSetLayoutBuilder::Make(const vk::Device & device) const
{
  // create descriptor set layout
  VkDescriptorSetLayoutCreateInfo dsetLayoutInfo{};
  dsetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  dsetLayoutInfo.bindingCount = static_cast<uint32_t>(m_uniformDescriptions.size());
  dsetLayoutInfo.pBindings = m_uniformDescriptions.data();

  VkDescriptorSetLayout descr_layout;
  if (auto res = vkCreateDescriptorSetLayout(device, &dsetLayoutInfo, nullptr, &descr_layout);
      res != VK_SUCCESS)
    throw std::runtime_error("Failed to create descriptor set layout");
  return vk::DescriptorSetLayout(descr_layout);
}

void DescriptorSetLayoutBuilder::Reset()
{
  m_uniformDescriptions.clear();
}

void DescriptorSetLayoutBuilder::DeclareDescriptor(uint32_t binding, VkDescriptorType type,
                                                   ShaderType shaderStagesMask)
{
  auto && uniformBinding = m_uniformDescriptions.emplace_back();
  uniformBinding.binding = binding;
  uniformBinding.descriptorType = type;
  uniformBinding.descriptorCount = 1;
  uniformBinding.stageFlags = CastInterfaceEnum2Vulkan<VkShaderStageFlagBits>(shaderStagesMask);
  uniformBinding.pImmutableSamplers = nullptr; // Optional
}

} // namespace RHI::vulkan::utils
