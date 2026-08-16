#pragma once

#include <mutex>
#include <variant>
#include <vector>

#include <CommandsExecution/CommandBuffer.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <Utils/DescriptorSetLayoutBuilder.hpp>
#include <Pipeline/UpdateDescriptorTask.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
struct DescriptorBufferLayout;
struct BufferUniform;
struct SamplerUniform;
struct SamplerArrayUniform;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct DescriptorBuffer final : public RHI::OwnedBy<Context>,
                                public OwnedBy<DescriptorBufferLayout>
{
  explicit DescriptorBuffer(Context & ctx, DescriptorBufferLayout & layout);
  ~DescriptorBuffer();
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  MAKE_ALIAS_FOR_GET_OWNER(DescriptorBufferLayout, GetLayout);
  DescriptorBuffer(DescriptorBuffer && rhs) noexcept;
  DescriptorBuffer & operator=(DescriptorBuffer && rhs) noexcept;

  void Invalidate();

  void UpdateDescriptor(UpdateDescriptorTask updateFunc) noexcept;

  void BindToCommandBuffer(details::CommandBuffer & commands, VkPipelineLayout pipelineLayout,
                           VkPipelineBindPoint bindPoint);

private:
  std::mutex m_setsLock;
  VkDescriptorPool m_pool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> m_sets;
  std::vector<VkDescriptorSetLayout> m_cachedLayouts;
  std::mutex m_updateDescriptorsLock;
  std::vector<UpdateDescriptorTask> m_updateTasks;
};

} // namespace RHI::vulkan
