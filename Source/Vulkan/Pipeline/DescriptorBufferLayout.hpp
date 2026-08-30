#pragma once

#include <span>
#include <vector>

#include <CommandsExecution/CommandBuffer.hpp>
#include <Memory/ResourceUser.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <Utils/DescriptorSetLayoutBuilder.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct Context;
struct Pipeline;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct DescriptorBufferLayout final : public OwnedBy<Context>
{
  explicit DescriptorBufferLayout(Context & ctx);
  ~DescriptorBufferLayout();
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public:
  void SetInvalid();
  void Invalidate();
  std::pair<VkDescriptorPool, std::vector<VkDescriptorSet>> AllocDescriptorSets() const;

  const std::vector<VkDescriptorSetLayout> & GetHandles() const & noexcept;

public:
  void DeclareDescriptorsArray(const LayoutIndex & index, VkDescriptorType type,
                               ShaderType shaderStage, uint32_t size);

private:
  std::vector<VkDescriptorSetLayout> m_layouts;
  std::vector<utils::DescriptorSetLayoutBuilder> m_builders;
  std::vector<VkDescriptorPoolSize> m_poolSizes;
};

} // namespace RHI::vulkan
