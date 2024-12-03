#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Utils/PipelineBuilder.hpp"
#include "../Utils/PipelineLayoutBuilder.hpp"
#include "DescriptorsBuffer.hpp"
#include "RenderPass.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

struct Pipeline final : public IPipeline
{
  explicit Pipeline(const Context & ctx, const RenderPass & renderPass, uint32_t subpassIndex);
  virtual ~Pipeline() override;

  virtual void AttachShader(ShaderType type, const std::filesystem::path & path) override;
  virtual void AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type) override;
  virtual void AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset,
                                 uint32_t elemsCount, InputAttributeElementType elemsType) override;
  virtual IBufferGPU * DeclareUniform(const char * name, uint32_t binding, ShaderType shaderStage,
                                      size_t size) override;
  virtual IImageGPU_Sampler * DeclareSampler(const char * name, uint32_t binding,
                                             ShaderType shaderStage) override;

  virtual void Invalidate() override;
  virtual uint32_t GetSubpass() const noexcept override { return m_subpassIndex; }

  vk::Pipeline GetPipelineHandle() const noexcept { return m_pipeline; }

  void Bind(const vk::CommandBuffer & buffer, VkPipelineBindPoint bindPoint);

private:
  const Context & m_owner;
  const RenderPass & m_renderPass;
  uint32_t m_subpassIndex;

  vk::PipelineLayout m_layout = VK_NULL_HANDLE;
  vk::Pipeline m_pipeline = VK_NULL_HANDLE;
  DescriptorBuffer m_descriptors;


  utils::PipelineLayoutBuilder m_layoutBuilder;
  utils::PipelineBuilder m_pipelineBuilder;
  bool m_invalidPipeline : 1 = false;
  bool m_invalidPipelineLayout : 1 = false;
};

} // namespace RHI::vulkan
