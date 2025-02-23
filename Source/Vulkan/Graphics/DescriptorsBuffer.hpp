#pragma once

#include <deque>

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Resources/BufferGPU.hpp"
#include "../Utils/DescriptorSetLayoutBuilder.hpp"
#include "Uniforms/BufferUniform.hpp"
#include "Uniforms/SamplerUniform.hpp"

namespace RHI::vulkan
{
struct Context;
struct SubpassConfiguration;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct DescriptorBuffer final : public RHI::OwnedBy<Context>,
                                public OwnedBy<SubpassConfiguration>
{
  explicit DescriptorBuffer(Context & ctx, SubpassConfiguration & owner);
  ~DescriptorBuffer();

  void Invalidate();

  void DeclareUniformsArray(uint32_t binding, ShaderType shaderStage, uint32_t size,
                            BufferUniform * out_array[]);
  void DeclareSamplersArray(uint32_t binding, ShaderType shaderStage, uint32_t size,
                            SamplerUniform * out_array[]);


  void OnDescriptorChanged(const BufferUniform & descriptor) noexcept;
  void OnDescriptorChanged(const SamplerUniform & descriptor) noexcept;

  void BindToCommandBuffer(const VkCommandBuffer & buffer, VkPipelineLayout pipelineLayout,
                           VkPipelineBindPoint bindPoint);

  VkDescriptorSetLayout GetLayoutHandle() const noexcept;
  VkDescriptorSet GetHandle() const noexcept;

  void TransitLayoutForUsedImages(details::CommandBuffer& commandBuffer);

private:
  using BufferUniforms = std::deque<BufferUniform>;
  using SamplerUniforms = std::deque<SamplerUniform>;

private:
  VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
  VkDescriptorSet m_set = VK_NULL_HANDLE;
  VkDescriptorPool m_pool = VK_NULL_HANDLE;

  std::unordered_map<VkDescriptorType, uint32_t> m_capacity;
  std::unordered_map<VkDescriptorType, BufferUniforms> m_bufferUniformDescriptors;
  std::unordered_map<VkDescriptorType, SamplerUniforms> m_samplerDescriptors;

  utils::DescriptorSetLayoutBuilder m_layoutBuilder;

  bool m_invalidLayout : 1 = false;
  bool m_invalidPool : 1 = false;
  bool m_invalidSet : 1 = false;

public:
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  MAKE_ALIAS_FOR_GET_OWNER(SubpassConfiguration, GetConfiguration);
};

} // namespace RHI::vulkan
