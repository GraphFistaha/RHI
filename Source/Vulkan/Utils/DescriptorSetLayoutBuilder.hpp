#pragma once
#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
struct DescriptorSetLayoutBuilder final
{
  vk::DescriptorSetLayout Make(const vk::Device & device) const;
  void Reset();
  void DeclareDescriptor(uint32_t binding, VkDescriptorType type, ShaderType shaderStage);

private:
  std::vector<VkDescriptorSetLayoutBinding> m_uniformDescriptions;
};
} // namespace RHI::vulkan::utils
