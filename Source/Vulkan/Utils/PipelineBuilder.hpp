#pragma once
#include <filesystem>
#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>


namespace RHI::vulkan::utils
{

/// @brief Utility-class to automatize pipeline building. It provides default values to many settings nad methods to configure your pipeline
struct PipelineBuilder final
{
  PipelineBuilder();
  RESTRICTED_COPY(PipelineBuilder);

public:
  VkPipeline Make(const VkDevice & device, const VkRenderPass & renderPass, uint32_t subpass_index,
                  const VkPipelineLayout & layout);
  void Reset();

public:
  // shaders
  void AttachShader(RHI::ShaderType type, const std::filesystem::path & path);

  // assembly
  void SetMeshTopology(MeshTopology topology);
  void SetPrimitiveRestartEnabled(bool value);

  // rasterization
  void SetLineWidth(float width);
  void SetPolygonMode(PolygonMode mode);
  void SetCullingMode(CullingMode mode, FrontFace front);

  // depth stencil
  void SetDepthTestEnabled(bool enabled);
  void SetDepthWriteMode(bool enabled);
  void SetDepthTestCompareOperator(CompareOperation op);

  void SetStencilTestEnabled(bool enabled);

  // blending
  void OnColorAttachmentHasBound();
  void SetBlendEnabled(uint32_t attachmentIdx, bool value);

  // VAO settings
  void AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type);
  void AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset, uint32_t elemsCount,
                         InputAttributeElementType type);

private:
  VkPipelineDynamicStateCreateInfo m_dynamicStatesInfo{};
  VkPipelineVertexInputStateCreateInfo m_vertexInputInfo{};
  VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyInfo{};
  VkPipelineViewportStateCreateInfo m_viewportInfo{};
  VkPipelineDepthStencilStateCreateInfo m_depthStencilInfo{};
  VkPipelineRasterizationStateCreateInfo m_rasterizationInfo{};
  VkPipelineMultisampleStateCreateInfo m_multisampleInfo{};
  VkPipelineColorBlendStateCreateInfo m_colorBlendInfo{};

  /// Attached shaders
  std::vector<std::pair<ShaderType, std::filesystem::path>> m_attachedShaders;

  std::vector<VkDynamicState> m_dynamicStates;
  std::vector<VkPipelineColorBlendAttachmentState> m_colorBlendAttachments;
  std::vector<VkVertexInputBindingDescription> m_bindings;
  std::vector<VkVertexInputAttributeDescription> m_attributes;
};

} // namespace RHI::vulkan::utils
