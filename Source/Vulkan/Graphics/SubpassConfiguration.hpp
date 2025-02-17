#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../ContextualObject.hpp"
#include "../Utils/PipelineBuilder.hpp"
#include "../Utils/PipelineLayoutBuilder.hpp"
#include "DescriptorsBuffer.hpp"

namespace RHI::vulkan
{
struct RenderPass;
struct Subpass;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct SubpassConfiguration final : public ISubpassConfiguration,
                                    public ContextualObject
{
  explicit SubpassConfiguration(Context & ctx, Subpass & owner, const RenderPass & renderPass,
                                uint32_t subpassIndex);
  virtual ~SubpassConfiguration() override;

public: // ISubpassConfiguration interface
  virtual void AttachShader(ShaderType type, const std::filesystem::path & path) override;
  virtual void SetAttachmentUsage(ShaderImageSlot slot, uint32_t binding) override;
  virtual void AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type) override;
  virtual void AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset,
                                 uint32_t elemsCount, InputAttributeElementType elemsType) override;
  virtual void DefinePushConstant(uint32_t size, ShaderType shaderStage) override;
  virtual IBufferUniformDescriptor * DeclareUniform(uint32_t binding,
                                                    ShaderType shaderStage) override;
  virtual ISamplerUniformDescriptor * DeclareSampler(uint32_t binding,
                                                     ShaderType shaderStage) override;
  virtual void DeclareSamplersArray(uint32_t binding, ShaderType shaderStage, uint32_t size,
                                    ISamplerUniformDescriptor * out_array[]) override;

  virtual uint32_t GetSubpass() const noexcept override { return m_subpassIndex; }

public: // IInvalidable Interface
  virtual void Invalidate() override;
  virtual void SetInvalid() override;

public: // public internal API
  VkPipeline GetPipelineHandle() const noexcept { return m_pipeline; }
  VkPipelineLayout GetPipelineLayoutHandle() const noexcept { return m_layout; }
  void BindToCommandBuffer(const VkCommandBuffer & buffer, VkPipelineBindPoint bindPoint);
  Subpass & GetSubpassOwner() & noexcept { return m_owner; }

private:
  const RenderPass & m_renderPass;
  Subpass & m_owner;

  uint32_t m_subpassIndex;

  std::optional<VkPushConstantRange> m_pushConstantRange = std::nullopt;
  VkPipelineLayout m_layout = VK_NULL_HANDLE;
  VkPipeline m_pipeline = VK_NULL_HANDLE;
  DescriptorBuffer m_descriptors;

  utils::PipelineLayoutBuilder m_layoutBuilder;
  utils::PipelineBuilder m_pipelineBuilder;
  bool m_invalidPipeline : 1 = false;
  bool m_invalidPipelineLayout : 1 = false;
};

} // namespace RHI::vulkan
