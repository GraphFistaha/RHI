#include "DescriptorsBuffer.hpp"

#include <algorithm>
#include <array>
#include <numeric>

#include "../Graphics/Subpass.hpp"
#include "../Resources/BufferGPU.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
#include "BufferUniform.hpp"
#include "DescriptorBufferLayout.hpp"
#include "SamplerUniform.hpp"


namespace RHI::vulkan::details
{

//discover https://kylehalladay.com/blog/tutorial/vulkan/2018/01/28/Textue-Arrays-Vulkan.html
template<typename UniformT>
  requires(std::is_same_v<UniformT, BufferUniform> || std::is_same_v<UniformT, SamplerUniform>)
void UpdateDescriptorResource(const Context & ctx, VkDescriptorSet set, const UniformT & uniform)
{
  assert(set);
  auto && info = uniform.CreateDescriptorInfo();

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = set;
  descriptorWrite.dstBinding = uniform.GetBinding();
  descriptorWrite.dstArrayElement = uniform.GetArrayIndex();
  descriptorWrite.descriptorType = uniform.GetDescriptorType();
  descriptorWrite.descriptorCount = 1;
  if constexpr (std::is_same_v<UniformT, BufferUniform>)
  {
    descriptorWrite.pBufferInfo = &info;
    descriptorWrite.pImageInfo = nullptr; // Optional
  }
  else if constexpr (std::is_same_v<UniformT, SamplerUniform>)
  {
    assert(uniform.GetDescriptorType() == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
           uniform.GetDescriptorType() == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
           uniform.GetDescriptorType() == VK_DESCRIPTOR_TYPE_SAMPLER);
    descriptorWrite.pBufferInfo = nullptr;
    descriptorWrite.pImageInfo = &info; // Optional
  }
  descriptorWrite.pTexelBufferView = nullptr; // Optional

  vkUpdateDescriptorSets(ctx.GetDevice(), 1, &descriptorWrite, 0, nullptr);
}


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

DescriptorBuffer::DescriptorBuffer(Context & ctx, const DescriptorBufferLayout & layout)
  : OwnedBy<Context>(ctx)
  , OwnedBy<const DescriptorBufferLayout>(layout)
{
}

DescriptorBuffer::~DescriptorBuffer()
{
  vkDestroyDescriptorPool(GetContext().GetDevice(), m_pool, nullptr);
}

void DescriptorBuffer::Invalidate()
{
  if (m_cachedLayouts != GetLayout().GetHandles())
  {
    auto [newPool, newSets] = GetLayout().AllocDescriptorSets();
    vkDestroyDescriptorPool(GetContext().GetDevice(), m_pool, nullptr);
    m_pool = newPool;
    m_sets = std::move(newSets);
    m_cachedLayouts = GetLayout().GetHandles();
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "VkDescriptorPool & Sets have been rebuilt");
  }
}


void DescriptorBuffer::BindToCommandBuffer(const VkCommandBuffer & buffer,
                                           VkPipelineLayout pipelineLayout,
                                           VkPipelineBindPoint bindPoint)
{
  {
    std::lock_guard lk{m_updateDescriptorsLock};
    for (const GenericUniformPtr & task : m_updateTasks)
    {
      std::visit(
        [this](auto && uniformPtr)
        {
          uint32_t setIdx = uniformPtr->GetSet();
          details::UpdateDescriptorResource(GetContext(), m_sets[setIdx], *uniformPtr);
        },
        task);
    }
    m_updateTasks.clear();
  }
  vkCmdBindDescriptorSets(buffer, bindPoint, pipelineLayout, 0,
                          static_cast<uint32_t>(m_sets.size()), m_sets.data(), 0, nullptr);
}

} // namespace RHI::vulkan
