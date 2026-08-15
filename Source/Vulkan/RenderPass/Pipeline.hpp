#pragma once

#include <CommandsExecution/CommandBuffer.hpp>
#include <Descriptors/DescriptorBufferLayout.hpp>
#include <Descriptors/DescriptorsBuffer.hpp>
#include <Private/OwnedBy.hpp>
#include <RenderPass/SubpassLayout.hpp>
#include <RHI.hpp>
#include <Utils/PipelineBuilder.hpp>
#include <Utils/PipelineLayoutBuilder.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
struct RenderPass;
struct PipelineProcess;
namespace details
{
struct CommandBuffer;
}
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct Pipeline final : public IPipeline,
                                    public OwnedBy<Context>,
                                    public OwnedBy<RenderPass>,
                                    public ICommandWriter
{
  explicit Pipeline(Context & ctx, RenderPass & owner, uint32_t subpassIndex, uint32_t familyQueue);
  virtual ~Pipeline() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  MAKE_ALIAS_FOR_GET_OWNER(RenderPass, GetRenderPass);

public: // IPipeline interface
  virtual void AttachShader(ShaderType type, const SpirV & spirv) override;
  virtual void BindAttachment(uint32_t binding, ShaderAttachmentSlot slot,
                              LayoutIndex inputIndex = LayoutIndex()) override;
  virtual void BindResolver(uint32_t binding, uint32_t resolve_for) override;
  virtual void AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type) override;
  virtual void AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset,
                                 uint32_t elemsCount, InputAttributeElementType elemsType) override;
  virtual void DefinePushConstant(uint32_t size, ShaderType shaderStage) override;
  virtual IBufferUniformDescriptor * DeclareUniform(LayoutIndex index,
                                                    ShaderType shaderStage) override;
  virtual ISamplerUniformDescriptor * DeclareSampler(LayoutIndex index,
                                                     ShaderType shaderStage) override;

  virtual void DeclareUniformsArray(LayoutIndex index, ShaderType shaderStage, uint32_t size,
                                    IBufferUniformDescriptor * outArray[]) override;
  /// Sampler2D / Sampler2DArray uniform
  virtual void DeclareSamplersArray(LayoutIndex index, ShaderType shaderStage, uint32_t size,
                                    ISamplerUniformDescriptor * outArray[]) override;

  virtual uint32_t GetSubpassIndex() const noexcept override { return m_subpassIndex; }
  virtual void SetMeshTopology(MeshTopology topology) noexcept override;

  virtual void EnableDepthTest(bool enabled) noexcept override;
  virtual void SetDepthFunc(CompareOperation op) noexcept override;

  virtual void SetRenderProcess(PipelineProcessPtr process) override;

public: //ICommandWriter
  virtual void RecordCommands(details::CommandBuffer & commands) override;

public: // IResourceUser
  void CollectResources(std::vector<ResourcePtr> & resources) const;
  void SynchroniseResources(details::CommandBuffer & commands) const;

public: // IInvalidable Interface
  virtual void Invalidate() override;
  virtual void SetInvalid() override;

public: // public internal API
  bool WaitForPipelineIsValid() const noexcept;
  VkPipeline GetPipelineHandle() const noexcept { return m_pipeline; }
  VkPipelineLayout GetPipelineLayoutHandle() const noexcept { return m_pipelineLayout; }
  void BindToCommandBuffer(details::CommandBuffer & commands, VkPipelineBindPoint bindPoint);
  void OnDescriptorChanged(UpdateDescriptorTask task) noexcept;
  void SetDirtyCommands() noexcept;
  const SubpassLayout & GetLayout() const & noexcept;
  SubpassLayout & GetLayout() & noexcept;
  const DescriptorBufferLayout & GetDescriptorsLayout() const & noexcept;
  DescriptorBufferLayout & GetDescriptorsLayout() & noexcept;
  DescriptorBuffer & GetDescriptorBuffer() & noexcept;

private:
  uint32_t m_subpassIndex;

  std::optional<VkPushConstantRange> m_pushConstantRange = std::nullopt;
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
  VkPipeline m_pipeline = VK_NULL_HANDLE;

  utils::PipelineLayoutBuilder m_pipelineLayoutBuilder;
  utils::PipelineBuilder m_pipelineBuilder;

  std::atomic_bool m_invalidPipeline = false;
  bool m_invalidPipelineLayout = false;

  std::shared_ptr<PipelineProcess> m_process;
  std::atomic_bool m_dirtyCommands = false;

  details::CommandBuffer m_execBuffer;
  details::CommandBuffer m_writeBuffer;
  DescriptorBufferLayout m_descriptorsLayout;
  DescriptorBuffer m_execDescriptorBuffer;
  DescriptorBuffer m_writeDescriptorBuffer;

  SubpassLayout m_layout{VK_PIPELINE_BIND_POINT_GRAPHICS};
};

} // namespace RHI::vulkan
