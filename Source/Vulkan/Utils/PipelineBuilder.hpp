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
  VkPipeline Make(const VkDevice & device, const VkRenderPass & renderPass,
                    uint32_t subpass_index, const VkPipelineLayout & layout);
  void Reset();

  void AttachShader(RHI::ShaderType type, const std::filesystem::path & path);
  void SetMeshTopology(MeshTopology topology) { m_topology = topology; }
  void SetLineWidth(float width) { m_lineWidth = width; }
  void SetPolygonMode(PolygonMode mode) { m_polygonMode = mode; }
  void SetCullingMode(CullingMode mode, FrontFace front)
  {
    m_cullingMode = mode;
    m_frontFace = front;
  }

  void SetBlendEnabled(bool value) { m_blendEnabled = value; }

  void AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type);
  void AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset, uint32_t elemsCount,
                         InputAttributeElementType type);

private:
  MeshTopology m_topology = MeshTopology::Triangle;

  float m_lineWidth = 1.0f;
  PolygonMode m_polygonMode;

  CullingMode m_cullingMode;
  FrontFace m_frontFace;

  bool m_blendEnabled;
  BlendOperation m_blendColorOp;
  BlendOperation m_blendAlphaOp;
  BlendFactor m_blendSrcColorFactor;
  BlendFactor m_blendDstColorFactor;
  BlendFactor m_blendSrcAlphaFactor;
  BlendFactor m_blendDstAlphaFactor;

  std::vector<std::pair<ShaderType, std::filesystem::path>> m_attachedShaders;

  std::vector<VkDynamicState> m_dynamicStates;
  std::vector<VkPipelineColorBlendAttachmentState> m_colorBlendAttachments;
  std::vector<VkVertexInputBindingDescription> m_bindings;
  std::vector<VkVertexInputAttributeDescription> m_attributes;

private:
  PipelineBuilder(const PipelineBuilder &) = delete;
  PipelineBuilder & operator=(const PipelineBuilder &) = delete;
};

} // namespace RHI::vulkan::utils
