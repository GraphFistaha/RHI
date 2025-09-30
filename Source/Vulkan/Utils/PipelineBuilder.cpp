#include "PipelineBuilder.hpp"

#include <Utils/CastHelper.hpp>
#include <Utils/ShaderCompiler.hpp>

namespace
{
constexpr VkFormat InputAttributeElementFormat2VulkanEnum(uint32_t elemsCount,
                                                          RHI::InputAttributeElementType type)
{
  switch (type)
  {
    case RHI::InputAttributeElementType::FLOAT:
    {
      constexpr VkFormat formats[] = {VK_FORMAT_UNDEFINED, VK_FORMAT_R32_SFLOAT,
                                      VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT,
                                      VK_FORMAT_R32G32B32A32_SFLOAT};
      return formats[elemsCount];
    }
    case RHI::InputAttributeElementType::DOUBLE:
    {
      constexpr VkFormat formats[] = {VK_FORMAT_UNDEFINED, VK_FORMAT_R64_SFLOAT,
                                      VK_FORMAT_R64G64_SFLOAT, VK_FORMAT_R64G64B64_SFLOAT,
                                      VK_FORMAT_R64G64B64A64_SFLOAT};
      return formats[elemsCount];
    }
    case RHI::InputAttributeElementType::UINT:
    {
      constexpr VkFormat formats[] = {VK_FORMAT_UNDEFINED, VK_FORMAT_R32_UINT,
                                      VK_FORMAT_R32G32_UINT, VK_FORMAT_R32G32B32_UINT,
                                      VK_FORMAT_R32G32B32A32_UINT};
      return formats[elemsCount];
    }
    case RHI::InputAttributeElementType::SINT:
    {
      constexpr VkFormat formats[] = {VK_FORMAT_UNDEFINED, VK_FORMAT_R32_SINT,
                                      VK_FORMAT_R32G32_SINT, VK_FORMAT_R32G32B32_SINT,
                                      VK_FORMAT_R32G32B32A32_SINT};
      return formats[elemsCount];
    }
    default:
      throw std::runtime_error("Failed to cast InputAttributeFormat to vulkan enum");
  }
}

constexpr VkPipelineColorBlendAttachmentState DefaultColorBlendConfig() noexcept
{
  VkPipelineColorBlendAttachmentState state{};
  state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  state.blendEnable = VK_FALSE;
  state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
  state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  state.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
  state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
  state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  state.alphaBlendOp = VK_BLEND_OP_ADD;             // Optionald
  return state;
}

} // namespace

namespace RHI::vulkan::utils
{
template<>
constexpr inline VkPrimitiveTopology CastInterfaceEnum2Vulkan<VkPrimitiveTopology, MeshTopology>(
  MeshTopology value)
{
  switch (value)
  {
    case MeshTopology::Point:
      return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case MeshTopology::Line:
      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case MeshTopology::LineStrip:
      return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case MeshTopology::Triangle:
      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case MeshTopology::TriangleFan:
      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    case MeshTopology::TriangleStrip:
      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    default:
      return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
  }
}

template<>
constexpr inline VkPolygonMode CastInterfaceEnum2Vulkan<VkPolygonMode, PolygonMode>(
  PolygonMode value)
{
  switch (value)
  {
    case PolygonMode::Fill:
      return VK_POLYGON_MODE_FILL;
    case PolygonMode::Line:
      return VK_POLYGON_MODE_LINE;
    case PolygonMode::Point:
      return VK_POLYGON_MODE_POINT;
    default:
      return VK_POLYGON_MODE_MAX_ENUM;
  }
}

template<>
constexpr inline VkCullModeFlags CastInterfaceEnum2Vulkan<VkCullModeFlags, CullingMode>(
  CullingMode value)
{
  switch (value)
  {
    case CullingMode::None:
      return VK_CULL_MODE_NONE;
    case CullingMode::FrontFace:
      return VK_CULL_MODE_FRONT_BIT;
    case CullingMode::BackFace:
      return VK_CULL_MODE_FRONT_BIT;
    case CullingMode::FrontAndBack:
      return VK_CULL_MODE_FRONT_AND_BACK;
    default:
      return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
  }
}

template<>
constexpr inline VkFrontFace CastInterfaceEnum2Vulkan<VkFrontFace, FrontFace>(FrontFace value)
{
  switch (value)
  {
    case FrontFace::CW:
      return VK_FRONT_FACE_CLOCKWISE;
    case FrontFace::CCW:
      return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    default:
      return VK_FRONT_FACE_MAX_ENUM;
  }
}

template<>
constexpr inline VkCompareOp CastInterfaceEnum2Vulkan<VkCompareOp, CompareOperation>(
  CompareOperation value)
{
  switch (value)
  {
    case CompareOperation::Never:
      return VK_COMPARE_OP_NEVER;
    case CompareOperation::Always:
      return VK_COMPARE_OP_ALWAYS;
    case CompareOperation::Equal:
      return VK_COMPARE_OP_EQUAL;
    case CompareOperation::NotEqual:
      return VK_COMPARE_OP_NOT_EQUAL;
    case CompareOperation::Less:
      return VK_COMPARE_OP_LESS;
    case CompareOperation::LessOrEqual:
      return VK_COMPARE_OP_LESS_OR_EQUAL;
    case CompareOperation::Greater:
      return VK_COMPARE_OP_GREATER;
    case CompareOperation::GreaterOrEqual:
      return VK_COMPARE_OP_GREATER_OR_EQUAL;
    default:
      return VK_COMPARE_OP_MAX_ENUM;
  }
}

} // namespace RHI::vulkan::utils


namespace RHI::vulkan::utils
{
PipelineBuilder::PipelineBuilder()
{
  Reset();
}


VkPipeline PipelineBuilder::Make(const VkDevice & device, const VkRenderPass & renderPass,
                                 uint32_t subpass_index, const VkPipelineLayout & layout)
{
  // update pointers on arrays
  {
    m_dynamicStatesInfo.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
    m_dynamicStatesInfo.pDynamicStates = m_dynamicStates.data();
  }

  {
    m_vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(m_bindings.size());
    m_vertexInputInfo.pVertexBindingDescriptions = m_bindings.data();
    m_vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_attributes.size());
    m_vertexInputInfo.pVertexAttributeDescriptions = m_attributes.data();
  }

  {
    m_colorBlendInfo.attachmentCount = static_cast<uint32_t>(m_colorBlendAttachments.size());
    m_colorBlendInfo.pAttachments = m_colorBlendAttachments.data();
  }

  // Build shaders
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  std::vector<VkShaderModule> compiledShaders;
  for (auto && [type, path] : m_attachedShaders)
  {
    auto && module = BuildShaderModule(device, path);
    compiledShaders.push_back(module);
    auto && info = shaderStages.emplace_back(VkPipelineShaderStageCreateInfo{});
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = CastInterfaceEnum2Vulkan<VkShaderStageFlagBits>(type);
    info.pName = "main";
    info.module = module;
  }

  // create pipeline
  VkGraphicsPipelineCreateInfo pipeline_info{};
  {
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.layout = layout;
    pipeline_info.renderPass = renderPass;
    pipeline_info.subpass = subpass_index;
    //states
    pipeline_info.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipeline_info.pStages = shaderStages.data();
    pipeline_info.pDynamicState = &m_dynamicStatesInfo;
    pipeline_info.pInputAssemblyState = &m_inputAssemblyInfo;
    pipeline_info.pVertexInputState = &m_vertexInputInfo;
    pipeline_info.pViewportState = &m_viewportInfo;
    pipeline_info.pRasterizationState = &m_rasterizationInfo;
    pipeline_info.pMultisampleState = &m_multisampleInfo;
    pipeline_info.pColorBlendState = &m_colorBlendInfo;
    pipeline_info.pDepthStencilState = &m_depthStencilInfo;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipeline_info.basePipelineIndex = -1;              // Optional
  }

  VkPipeline pipeline{};
  if (auto res =
        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline);
      res != VK_SUCCESS)
    throw std::runtime_error("Failed to create graphics pipeline - ");

  // cleanup shader modules
  for (auto && shader : compiledShaders)
    vkDestroyShaderModule(device, shader, nullptr);

  return pipeline;
}

void PipelineBuilder::Reset()
{
  // fill arrays by default
  m_attachedShaders.clear();
  m_bindings.clear();
  m_attributes.clear();
  m_dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  m_colorBlendAttachments.clear();

  // dynamic states
  {
    m_dynamicStatesInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    m_dynamicStatesInfo.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
    m_dynamicStatesInfo.pDynamicStates = m_dynamicStates.data();
  }

  // vertex input
  {
    m_vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    m_vertexInputInfo.vertexBindingDescriptionCount = 0;
    m_vertexInputInfo.pVertexBindingDescriptions = nullptr;
    m_vertexInputInfo.vertexAttributeDescriptionCount = 0;
    m_vertexInputInfo.pVertexAttributeDescriptions = nullptr;
  }

  // input assembly
  {
    m_inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
  }

  // viewport
  {
    m_viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    m_viewportInfo.viewportCount = 1;
    m_viewportInfo.scissorCount = 1;
  }

  // multisampling
  {
    m_multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_multisampleInfo.sampleShadingEnable = VK_FALSE;
    m_multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    m_multisampleInfo.minSampleShading = 1.0f;          // Optional
    m_multisampleInfo.pSampleMask = nullptr;            // Optional
    m_multisampleInfo.alphaToCoverageEnable = VK_FALSE; // Optional
    m_multisampleInfo.alphaToOneEnable = VK_FALSE;      // Optional
  }

  // color blending
  {
    m_colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_colorBlendInfo.logicOpEnable = VK_FALSE;
    m_colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
    m_colorBlendInfo.blendConstants[0] = 0.0f;   // Optional
    m_colorBlendInfo.blendConstants[1] = 0.0f;   // Optional
    m_colorBlendInfo.blendConstants[2] = 0.0f;   // Optional
    m_colorBlendInfo.blendConstants[3] = 0.0f;   // Optional
    m_colorBlendInfo.attachmentCount = 0;
    m_colorBlendInfo.pAttachments = nullptr;
  }

  // depth stencil
  {
    m_depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_depthStencilInfo.depthTestEnable = VK_FALSE;
    m_depthStencilInfo.depthWriteEnable = VK_TRUE;
    m_depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    m_depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    m_depthStencilInfo.stencilTestEnable = VK_FALSE;
  }

  // rasterization
  {
    m_rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_rasterizationInfo.depthClampEnable = VK_FALSE;
    m_rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    m_rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    m_rasterizationInfo.lineWidth = 1.0f;
    m_rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    m_rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    m_rasterizationInfo.depthBiasEnable = VK_FALSE;
    m_rasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
    m_rasterizationInfo.depthBiasClamp = 0.0f;          // Optional
    m_rasterizationInfo.depthBiasSlopeFactor = 0.0f;    // Optional
  }
}

void PipelineBuilder::AttachShader(RHI::ShaderType type, const std::filesystem::path & path)
{
  m_attachedShaders.push_back({type, path});
}

void PipelineBuilder::SetSamplesCount(RHI::SamplesCount samplesCount)
{
  m_cachedSamplesCount = samplesCount;
  m_multisampleInfo.rasterizationSamples =
    utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(samplesCount);
}

RHI::SamplesCount PipelineBuilder::GetSamplesCount() const noexcept
{
  return m_cachedSamplesCount;
}

void PipelineBuilder::SetMeshTopology(MeshTopology topology)
{
  m_inputAssemblyInfo.topology = utils::CastInterfaceEnum2Vulkan<VkPrimitiveTopology>(topology);
}

void PipelineBuilder::SetPrimitiveRestartEnabled(bool value)
{
  m_inputAssemblyInfo.primitiveRestartEnable = value ? VK_TRUE : VK_FALSE;
}

void PipelineBuilder::SetLineWidth(float width)
{
  m_rasterizationInfo.lineWidth = width;
}

void PipelineBuilder::SetPolygonMode(PolygonMode mode)
{
  m_rasterizationInfo.polygonMode = utils::CastInterfaceEnum2Vulkan<VkPolygonMode>(mode);
}

void PipelineBuilder::SetCullingMode(CullingMode mode, FrontFace front)
{
  m_rasterizationInfo.cullMode = utils::CastInterfaceEnum2Vulkan<VkCullModeFlags>(mode);
  m_rasterizationInfo.frontFace = utils::CastInterfaceEnum2Vulkan<VkFrontFace>(front);
}

void PipelineBuilder::OnColorAttachmentHasBound()
{
  m_colorBlendAttachments.push_back(DefaultColorBlendConfig());
}

void PipelineBuilder::SetBlendEnabled(uint32_t attachmentIdx, bool value)
{
  m_colorBlendAttachments[attachmentIdx].blendEnable = value;
}

void PipelineBuilder::SetDepthTestEnabled(bool enabled)
{
  m_depthStencilInfo.depthTestEnable = enabled;
}

void PipelineBuilder::SetDepthWriteMode(bool enabled)
{
  m_depthStencilInfo.depthWriteEnable = enabled;
}

void PipelineBuilder::SetDepthTestCompareOperator(CompareOperation op)
{
  m_depthStencilInfo.depthCompareOp = utils::CastInterfaceEnum2Vulkan<VkCompareOp>(op);
}

void PipelineBuilder::SetStencilTestEnabled(bool enabled)
{
  m_depthStencilInfo.stencilTestEnable = enabled;
}

void PipelineBuilder::AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type)
{
  auto && bindingDescription = m_bindings.emplace_back();
  bindingDescription.binding = slot;
  bindingDescription.stride = stride;
  bindingDescription.inputRate = CastInterfaceEnum2Vulkan<VkVertexInputRate>(type);
}

void PipelineBuilder::AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset,
                                        uint32_t elemsCount, InputAttributeElementType type)
{
  auto && attrDescription = m_attributes.emplace_back();
  attrDescription.binding = binding;
  attrDescription.location = location;
  attrDescription.offset = offset;
  attrDescription.format = ::InputAttributeElementFormat2VulkanEnum(elemsCount, type);
}

} // namespace RHI::vulkan::utils
