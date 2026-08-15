#pragma once

#include <deque>
#include <span>

#include <CommandsExecution/CommandBuffer.hpp>
#include <Descriptors/BufferUniform.hpp>
#include <Descriptors/SamplerArrayUniform.hpp>
#include <Descriptors/SamplerUniform.hpp>
#include <Memory/ResourceUser.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <Utils/DescriptorSetLayoutBuilder.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
struct Subpass;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct DescriptorMetaInfo
{
  LayoutIndex index;
  uint32_t arraySize;
};

struct DescriptorBufferLayout final : public OwnedBy<Context>,
                                      public OwnedBy<Subpass>
{
  explicit DescriptorBufferLayout(Context & ctx, Subpass & owner);
  ~DescriptorBufferLayout();
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  MAKE_ALIAS_FOR_GET_OWNER(Subpass, GetSubpass);

  void DeclareBufferUniformsArray(LayoutIndex index, ShaderType shaderStage, uint32_t size,
                                  IBufferUniformDescriptor * outArray[]);
  void DeclareSamplerUniformsArray(LayoutIndex index, ShaderType shaderStage, uint32_t size,
                                   ISamplerUniformDescriptor * outArray[]);
  void DeclareSamplerArrayUniformsArray(LayoutIndex index, ShaderType shaderStage, uint32_t size,
                                        ISamplerArrayUniformDescriptor * outArray[]);
  void DeclareInputAttachmentUniform(LayoutIndex index, ShaderType shaderStage);

public:
  void CollectResources(std::vector<ResourcePtr> & resources) const;
  void SynchroniseResources(details::CommandBuffer & commands) const;

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
  std::unordered_map<LayoutIndex, std::vector<details::BaseDescriptor *>> m_indexedDescriptors;
};

} // namespace RHI::vulkan
