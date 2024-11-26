#pragma once

#include "../Resources/ImageGPU.hpp"
#include "../VulkanContext.hpp"

namespace RHI::vulkan
{
namespace details
{
struct DescriptorSetLayoutBuilder;
}

struct DescriptorBuffer final
{
  explicit DescriptorBuffer(const Context & ctx);
  ~DescriptorBuffer();

  void Invalidate();

  IBufferGPU * DeclareUniform(uint32_t binding, ShaderType shaderStage, size_t size);
  IImageGPU_Sampler * DeclareSampler(uint32_t binding, ShaderType shaderStage);

  void Bind(const vk::CommandBuffer & buffer, vk::PipelineLayout pipelineLayout,
            VkPipelineBindPoint bindPoint);
  vk::DescriptorSetLayout GetLayoutHandle() const noexcept;
  vk::DescriptorSet GetHandle() const noexcept;

private:
  using BufferDescriptor = std::pair<uint32_t /*binding*/, std::unique_ptr<BufferGPU>>;
  using BufferDescriptors = std::vector<BufferDescriptor>;

  using ImageDescriptor = std::pair<uint32_t /*binding*/, std::unique_ptr<IImageGPU_Sampler>>;
  using ImageDescriptors = std::vector<ImageDescriptor>;

private:
  const Context & m_owner;

  vk::DescriptorSetLayout m_layout = VK_NULL_HANDLE;
  vk::DescriptorSet m_set = VK_NULL_HANDLE;
  vk::DescriptorPool m_pool = VK_NULL_HANDLE;

  std::unordered_map<VkDescriptorType, uint32_t> m_capacity;
  std::unordered_map<VkDescriptorType, BufferDescriptors> m_bufferDescriptors;
  std::unordered_map<VkDescriptorType, ImageDescriptors> m_imageDescriptors;

  std::unique_ptr<details::DescriptorSetLayoutBuilder> m_layoutBuilder;

  bool m_invalidLayout : 1 = false;
  bool m_invalidPool : 1 = false;
  bool m_invalidSet : 1 = false;
};

} // namespace RHI::vulkan
