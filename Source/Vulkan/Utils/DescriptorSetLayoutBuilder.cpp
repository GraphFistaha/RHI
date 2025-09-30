#include "DescriptorSetLayoutBuilder.hpp"

#include <Utils/CastHelper.hpp>

namespace RHI::vulkan::utils
{
VkDescriptorSetLayout DescriptorSetLayoutBuilder::Make(const VkDevice & device) const
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
  return VkDescriptorSetLayout(descr_layout);
}

void DescriptorSetLayoutBuilder::Reset()
{
  m_uniformDescriptions.clear();
}

void DescriptorSetLayoutBuilder::DeclareDescriptor(uint32_t binding, VkDescriptorType type,
                                                   ShaderType shaderStagesMask)
{
  DeclareDescriptorsArray(binding, type, shaderStagesMask, 1);
}

void DescriptorSetLayoutBuilder::DeclareDescriptorsArray(uint32_t binding, VkDescriptorType type,
                                                         ShaderType shaderStage, uint32_t size)
{
  if (binding != m_uniformDescriptions.size())
    throw std::runtime_error("Uniforms must be declared in order of ascending binding index");
  auto && uniformBinding = m_uniformDescriptions.emplace_back();
  uniformBinding.binding = binding;
  uniformBinding.descriptorType = type;
  uniformBinding.descriptorCount = size;
  uniformBinding.stageFlags = CastInterfaceEnum2Vulkan<VkShaderStageFlagBits>(shaderStage);
  uniformBinding.pImmutableSamplers = nullptr; // Optional
}

} // namespace RHI::vulkan::utils
