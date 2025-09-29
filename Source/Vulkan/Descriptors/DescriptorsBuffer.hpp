#pragma once

#include <variant>
#include <vector>

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../CommandsExecution/CommandBuffer.hpp"
#include "../Resources/BufferGPU.hpp"
#include "../Utils/DescriptorSetLayoutBuilder.hpp"

namespace RHI::vulkan
{
struct Context;
struct DescriptorBufferLayout;
struct BufferUniform;
struct SamplerUniform;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct DescriptorBuffer final : public RHI::OwnedBy<Context>,
                                public OwnedBy<const DescriptorBufferLayout>
{
  explicit DescriptorBuffer(Context & ctx, const DescriptorBufferLayout & layout);
  ~DescriptorBuffer();
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  MAKE_ALIAS_FOR_GET_OWNER(const DescriptorBufferLayout, GetLayout);

  void Invalidate();

  template<typename DescriptorT>
  void UpdateDescriptor(const DescriptorT & descriptor) noexcept
  {
    std::lock_guard lk{m_updateDescriptorsLock};
    m_updateTasks.emplace_back(&descriptor);
  }

  void BindToCommandBuffer(const VkCommandBuffer & buffer, VkPipelineLayout pipelineLayout,
                           VkPipelineBindPoint bindPoint);

private:
  using GenericUniformPtr = std::variant<const BufferUniform *, const SamplerUniform *>;

  VkDescriptorPool m_pool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> m_sets;
  std::vector<VkDescriptorSetLayout> m_cachedLayouts;
  std::mutex m_updateDescriptorsLock;
  std::vector<GenericUniformPtr> m_updateTasks;
};

} // namespace RHI::vulkan
