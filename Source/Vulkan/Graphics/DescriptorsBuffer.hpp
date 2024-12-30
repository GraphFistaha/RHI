#pragma once

#include <deque>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Resources/BufferGPU.hpp"
#include "../Utils/DescriptorSetLayoutBuilder.hpp"
#include "BufferUniform.hpp"
#include "SamplerUniform.hpp"

namespace RHI::vulkan
{
struct Context;
struct Pipeline;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct DescriptorBuffer final
{
  explicit DescriptorBuffer(const Context & ctx, Pipeline & owner);
  ~DescriptorBuffer();

  void Invalidate();

  BufferUniform * DeclareUniform(uint32_t binding, ShaderType shaderStage);
  ImageSampler * DeclareSampler(uint32_t binding, ShaderType shaderStage);
  void OnDescriptorChanged(const BufferUniform & descriptor) noexcept;
  void OnDescriptorChanged(const ImageSampler & descriptor) noexcept;

  void BindToCommandBuffer(const vk::CommandBuffer & buffer, vk::PipelineLayout pipelineLayout,
                           VkPipelineBindPoint bindPoint);

  vk::DescriptorSetLayout GetLayoutHandle() const noexcept;
  vk::DescriptorSet GetHandle() const noexcept;

private:
  using BufferUniforms = std::deque<BufferUniform>;
  using SamplerUniforms = std::deque<ImageSampler>;

private:
  const Context & m_context;
  Pipeline & m_owner;

  vk::DescriptorSetLayout m_layout = VK_NULL_HANDLE;
  vk::DescriptorSet m_set = VK_NULL_HANDLE;
  vk::DescriptorPool m_pool = VK_NULL_HANDLE;

  std::unordered_map<VkDescriptorType, uint32_t> m_capacity;
  std::unordered_map<VkDescriptorType, BufferUniforms> m_bufferUniformDescriptors;
  std::unordered_map<VkDescriptorType, SamplerUniforms> m_samplerDescriptors;

  utils::DescriptorSetLayoutBuilder m_layoutBuilder;

  bool m_invalidLayout : 1 = false;
  bool m_invalidPool : 1 = false;
  bool m_invalidSet : 1 = false;
};

} // namespace RHI::vulkan
