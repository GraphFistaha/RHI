#pragma once

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Descriptors/DescriptorBufferLayout.hpp"
#include "../Utils/PipelineBuilder.hpp"
#include "../Utils/PipelineLayoutBuilder.hpp"

namespace RHI::vulkan
{
struct Context;
struct RenderPass;
struct Subpass;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct SubpassConfiguration final : public ISubpassConfiguration,
                                    public OwnedBy<Context>,
                                    public OwnedBy<Subpass>
{
  explicit SubpassConfiguration(Context & ctx, Subpass & owner, uint32_t subpassIndex);
  virtual ~SubpassConfiguration() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  MAKE_ALIAS_FOR_GET_OWNER(Subpass, GetSubpass);

public: // ISubpassConfiguration interface
  virtual void AttachShader(ShaderType type, const std::filesystem::path & path) override;
  virtual void BindAttachment(uint32_t binding, ShaderAttachmentSlot slot) override;
  virtual void BindResolver(uint32_t binding, uint32_t resolve_for) override;
  virtual void AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type) override;
  virtual void AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset,
                                 uint32_t elemsCount, InputAttributeElementType elemsType) override;
  virtual void DefinePushConstant(uint32_t size, ShaderType shaderStage) override;
  virtual IBufferUniformDescriptor * DeclareUniform(LayoutIndex index,
                                                    ShaderType shaderStage) override;
  virtual ISamplerUniformDescriptor * DeclareSampler(LayoutIndex index,
                                                     ShaderType shaderStage) override;
  virtual void DeclareSamplersArray(LayoutIndex index, ShaderType shaderStage, uint32_t size,
                                    ISamplerUniformDescriptor * out_array[]) override;

  virtual uint32_t GetSubpassIndex() const noexcept override { return m_subpassIndex; }
  virtual void SetMeshTopology(MeshTopology topology) noexcept override;

  virtual void EnableDepthTest(bool enabled) noexcept override;
  virtual void SetDepthFunc(CompareOperation op) noexcept override;

public: // IInvalidable Interface
  virtual void Invalidate() override;
  virtual void SetInvalid() override;

public: // public internal API
  void WaitForPipelineIsValid() const noexcept;
  VkPipeline GetPipelineHandle() const noexcept { return m_pipeline; }
  VkPipelineLayout GetPipelineLayoutHandle() const noexcept { return m_pipelineLayout; }
  const DescriptorBufferLayout & GetDescriptorsLayout() const & noexcept;
  void BindToCommandBuffer(const VkCommandBuffer & buffer, VkPipelineBindPoint bindPoint);
  void TransitLayoutForUsedImages(details::CommandBuffer & commandBuffer);

private:
  uint32_t m_subpassIndex;

  std::optional<VkPushConstantRange> m_pushConstantRange = std::nullopt;
  DescriptorBufferLayout m_descriptorsLayout;
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
  VkPipeline m_pipeline = VK_NULL_HANDLE;

  utils::PipelineLayoutBuilder m_pipelineLayoutBuilder;
  utils::PipelineBuilder m_pipelineBuilder;

  std::atomic_bool m_invalidPipeline = false;
  bool m_invalidPipelineLayout = false;
};

} // namespace RHI::vulkan
