#pragma once

#include <deque>
#include <span>

#include <CommandsExecution/CommandBuffer.hpp>
#include <Descriptors/BufferUniform.hpp>
#include <Descriptors/SamplerArrayUniform.hpp>
#include <Descriptors/SamplerUniform.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <Utils/DescriptorSetLayoutBuilder.hpp>
#include <vulkan/vulkan.hpp>

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
  void DeclareSamplerArrayUniformsArray(LayoutIndex index, ShaderType shaderStage, uint32_t size,
                                        ISamplerArrayUniformDescriptor * outArray[]);

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
  using SamplerArrayUniforms = std::deque<SamplerArrayUniform>;

  std::vector<VkDescriptorSetLayout> m_layouts;
  std::vector<utils::DescriptorSetLayoutBuilder> m_builders;
  std::vector<ValidityFlag> m_invalidLayouts;

  BufferUniforms m_bufferUniformDescriptors;
  SamplerUniforms m_samplerDescriptors;
  SamplerArrayUniforms m_samplerArrayDescriptors;
  std::unordered_map<VkDescriptorType, std::vector<DescriptorMetaInfo>> m_setsInfo;
  std::unordered_map<LayoutIndex, std::vector<details::BaseUniform *>> m_indexedDescriptors;
};

} // namespace RHI::vulkan
