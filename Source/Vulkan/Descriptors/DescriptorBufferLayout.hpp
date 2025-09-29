#pragma once

#include <deque>
#include <span>

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../CommandsExecution/CommandBuffer.hpp"
#include "../Utils/DescriptorSetLayoutBuilder.hpp"
#include "BufferUniform.hpp"
#include "SamplerUniform.hpp"

namespace RHI::vulkan
{
struct Context;
struct SubpassConfiguration;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct DescriptorMetaInfo
{
  LayoutIndex index;
  uint32_t arraySize;
};

struct DescriptorBufferLayout final : public OwnedBy<Context>,
                                      public OwnedBy<SubpassConfiguration>
{
  explicit DescriptorBufferLayout(Context & ctx, SubpassConfiguration & owner);
  ~DescriptorBufferLayout();
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  MAKE_ALIAS_FOR_GET_OWNER(SubpassConfiguration, GetConfiguration);

  void DeclareBufferUniformsArray(LayoutIndex index, ShaderType shaderStage, uint32_t size,
                                  IBufferUniformDescriptor * outArray[]);
  void DeclareSamplerUniformsArray(LayoutIndex index, ShaderType shaderStage, uint32_t size,
                                   ISamplerUniformDescriptor * outArray[]);

  template<typename DescriptorT>
  void OnDescriptorChanged(const DescriptorT & uniform) noexcept
  {
    GetConfiguration().GetSubpass().OnDescriptorChanged(uniform);
  }

  void TransitLayoutForUsedImages(details::CommandBuffer & commandBuffer);

public:
  void SetInvalid();
  void Invalidate();
  std::pair<VkDescriptorPool, std::vector<VkDescriptorSet>> AllocDescriptorSets() const;
  uint32_t GetCountOfOneTypeDescriptors(VkDescriptorType type) const;

  const std::vector<VkDescriptorSetLayout> & GetHandles() const & noexcept;

private:
  void DeclareDescriptorsArray(const LayoutIndex & index, VkDescriptorType type,
                               ShaderType shaderStage, uint32_t size);

private:
  enum class ValidityFlag : uint8_t
  {
    Valid,
    NotValid
  };
  using BufferUniforms = std::deque<BufferUniform>;
  using SamplerUniforms = std::deque<SamplerUniform>;

  std::vector<VkDescriptorSetLayout> m_layouts;
  std::vector<utils::DescriptorSetLayoutBuilder> m_builders;
  std::vector<ValidityFlag> m_invalidLayouts;

  BufferUniforms m_bufferUniformDescriptors;
  SamplerUniforms m_samplerDescriptors;
  std::unordered_map<VkDescriptorType, std::vector<DescriptorMetaInfo>> m_setsInfo;
  std::unordered_map<LayoutIndex, std::vector<details::BaseUniform *>> m_indexedDescriptors;
};

} // namespace RHI::vulkan
