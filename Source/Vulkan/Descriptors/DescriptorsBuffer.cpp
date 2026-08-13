#include "DescriptorsBuffer.hpp"

#include <algorithm>
#include <array>
#include <numeric>

#include <Descriptors/BufferUniform.hpp>
#include <Descriptors/DescriptorBufferLayout.hpp>
#include <Descriptors/InputAttachmentUniform.hpp>
#include <Descriptors/SamplerUniform.hpp>
#include <Memory/BufferGPU.hpp>
#include <RenderPass/Subpass.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>


namespace RHI::vulkan::details
{

constexpr RHI::BufferGPUUsage DescriptorType2BufferUsage(VkDescriptorType type)
{
  switch (type)
  {
    /*case VK_DESCRIPTOR_TYPE_SAMPLER:
      return VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;*/
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      return BufferGPUUsage::UniformBuffer;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      return BufferGPUUsage::StorageBuffer;
    default:
      throw std::runtime_error("Failed to cast DescriptorType to BufferUsage");
  }
}
} // namespace RHI::vulkan::details


namespace RHI::vulkan
{

DescriptorBuffer::DescriptorBuffer(Context & ctx, DescriptorBufferLayout & layout)
  : OwnedBy<Context>(ctx)
  , OwnedBy<DescriptorBufferLayout>(layout)
{
}

DescriptorBuffer::~DescriptorBuffer()
{
  vkDestroyDescriptorPool(GetContext().GetGpuConnection().GetDevice(), m_pool, nullptr);
}

DescriptorBuffer::DescriptorBuffer(DescriptorBuffer && rhs) noexcept
  : OwnedBy<Context>(std::move(rhs))
  , OwnedBy<DescriptorBufferLayout>(std::move(rhs))
{
  std::swap(m_pool, rhs.m_pool);
  std::swap(m_sets, rhs.m_sets);
  std::swap(m_cachedLayouts, rhs.m_cachedLayouts);
  std::swap(m_updateTasks, rhs.m_updateTasks);
}

DescriptorBuffer & DescriptorBuffer::operator=(DescriptorBuffer && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<Context>::operator=(std::move(rhs));
    OwnedBy<DescriptorBufferLayout>::operator=(std::move(rhs));
    std::swap(m_pool, rhs.m_pool);
    std::swap(m_sets, rhs.m_sets);
    std::swap(m_cachedLayouts, rhs.m_cachedLayouts);
    std::swap(m_updateTasks, rhs.m_updateTasks);
  }
  return *this;
}

void DescriptorBuffer::Invalidate()
{
  if (m_cachedLayouts != GetLayout().GetHandles())
  {
    auto [newPool, newSets] = GetLayout().AllocDescriptorSets();
    std::lock_guard lk{m_setsLock};
    vkDestroyDescriptorPool(GetContext().GetGpuConnection().GetDevice(), m_pool, nullptr);
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG,
                     "VkDescriptorPool({}) & Sets have been rebuilt - {}",
                     static_cast<void *>(m_pool), static_cast<void *>(newPool));
    m_pool = newPool;
    m_sets = std::move(newSets);
    m_cachedLayouts = GetLayout().GetHandles();
  }
}

void DescriptorBuffer::UpdateDescriptor(UpdateDescriptorTask updateFunc) noexcept
{
  std::lock_guard lk{m_updateDescriptorsLock};
  m_updateTasks.emplace_back(std::move(updateFunc));
}


void DescriptorBuffer::BindToCommandBuffer(details::CommandBuffer & commands,
                                           VkPipelineLayout pipelineLayout,
                                           VkPipelineBindPoint bindPoint)
{
  std::lock_guard lk{m_setsLock};
  if (m_sets.empty())
    return;

  assert(!m_sets.empty());
  {
    std::lock_guard lk{m_updateDescriptorsLock};
    for (auto && task : m_updateTasks)
      task(GetContext(), m_sets);
    m_updateTasks.clear();
  }
  commands.PushCommand(vkCmdBindDescriptorSets, bindPoint, pipelineLayout, 0,
                       static_cast<uint32_t>(m_sets.size()), m_sets.data(), 0, nullptr);
}

} // namespace RHI::vulkan
