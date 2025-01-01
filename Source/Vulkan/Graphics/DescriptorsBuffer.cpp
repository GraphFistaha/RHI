#include "DescriptorsBuffer.hpp"

#include <algorithm>
#include <array>
#include <numeric>

#include "../Resources/BufferGPU.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"
#include "Pipeline.hpp"
#include "Subpass.hpp"


namespace RHI::vulkan
{
namespace details
{

vk::DescriptorPool CreateDescriptorPool(
  const Context & ctx, const std::unordered_map<VkDescriptorType, uint32_t> & distribution)
{
  assert(!distribution.empty());
  std::vector<VkDescriptorPoolSize> poolSizes;
  for (auto [type, count] : distribution)
  {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = type;
    poolSize.descriptorCount = count;
    poolSizes.push_back(poolSize);
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = std::accumulate(poolSizes.begin(), poolSizes.end(), 0u,
                                     [](uint32_t acc, const VkDescriptorPoolSize & pool_size)
                                     { return acc + pool_size.descriptorCount; });
  assert(poolInfo.maxSets != 0);

  VkDescriptorPool c_pool;
  if (vkCreateDescriptorPool(ctx.GetDevice(), &poolInfo, nullptr, &c_pool) != VK_SUCCESS)
    throw std::runtime_error("failed to create descriptor pool!");
  return vk::DescriptorPool(c_pool);
}

vk::DescriptorSet CreateDescriptorSet(const Context & ctx, VkDescriptorPool pool,
                                      VkDescriptorSetLayout layout)
{
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &layout;

  VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
  if (vkAllocateDescriptorSets(ctx.GetDevice(), &allocInfo, &descriptor_set) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate descriptor sets!");
  return vk::DescriptorSet(descriptor_set);
}


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
} // namespace details


DescriptorBuffer::DescriptorBuffer(const Context & ctx, Pipeline & owner)
  : m_context(ctx)
  , m_owner(owner)
{
}

DescriptorBuffer::~DescriptorBuffer()
{
  if (m_layout)
    vkDestroyDescriptorSetLayout(m_context.GetDevice(), m_layout, nullptr);
  if (m_pool)
    vkDestroyDescriptorPool(m_context.GetDevice(), m_pool, nullptr);
}

void DescriptorBuffer::Invalidate()
{
  if (m_capacity.empty())
    return;

  if (m_invalidLayout || !m_layout)
  {
    auto new_layout = m_layoutBuilder.Make(m_context.GetDevice());
    if (!!m_layout)
      vkDestroyDescriptorSetLayout(m_context.GetDevice(), m_layout, nullptr);
    m_layout = new_layout;
    m_invalidLayout = false;
    m_invalidSet = true;
  }

  if (m_invalidPool || !m_pool)
  {
    auto new_pool = details::CreateDescriptorPool(m_context, m_capacity);
    if (!!m_pool)
    {
      vkDestroyDescriptorPool(m_context.GetDevice(), m_pool, nullptr);
      m_set = VK_NULL_HANDLE;
    }
    m_pool = new_pool;
    m_invalidPool = false;
    m_invalidSet = true;
  }

  if (m_invalidSet || !m_set)
  {
    auto new_set = details::CreateDescriptorSet(m_context, m_pool, m_layout);

    if (m_set)
    {
      VkDescriptorSet deleting_set = m_set;
      vkFreeDescriptorSets(m_context.GetDevice(), m_pool, 1, &deleting_set);
    }
    m_set = new_set;
    m_invalidSet = false;
  }

  for (auto && [type, oneTypeDescriptors] : m_bufferUniformDescriptors)
  {
    for (auto && descriptor : oneTypeDescriptors)
      details::UpdateDescriptorResource(m_context, m_set, descriptor);
  }

  for (auto && [type, oneTypeDescriptors] : m_samplerDescriptors)
  {
    for (auto && descriptor : oneTypeDescriptors)
      details::UpdateDescriptorResource(m_context, m_set, descriptor);
  }

  m_owner.GetSubpassOwner().SetDirtyCacheCommands();
}

void DescriptorBuffer::DeclareUniformsArray(uint32_t binding, ShaderType shaderStage, uint32_t size,
                                            BufferUniform * out_array[])
{
  const VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  auto && descriptors = m_bufferUniformDescriptors[type];
  m_layoutBuilder.DeclareDescriptorsArray(binding, type, shaderStage, size);
  m_invalidLayout = true;
  m_capacity[type] += size;

  for (uint32_t i = 0; i < size; ++i)
  {
    BufferUniform new_uniform(m_context, *this, type, binding, i);
    auto && uniform = descriptors.emplace_back(std::move(new_uniform));
    out_array[i] = &uniform;
  }
}

void DescriptorBuffer::DeclareSamplersArray(uint32_t binding, ShaderType shaderStage, uint32_t size,
                                            SamplerUniform * out_array[])
{
  const VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  auto && descriptors = m_samplerDescriptors[type];
  m_layoutBuilder.DeclareDescriptorsArray(binding, type, shaderStage, size);
  m_invalidLayout = true;

  m_capacity[type] += size;
  for (uint32_t i = 0; i < size; ++i)
  {
    SamplerUniform new_uniform(m_context, *this, type, binding, i);
    auto && uniform = descriptors.emplace_back(std::move(new_uniform));
    out_array[i] = &uniform;
  }
}

void DescriptorBuffer::OnDescriptorChanged(const BufferUniform & descriptor) noexcept
{
  if (m_set)
  {
    details::UpdateDescriptorResource(m_context, m_set, descriptor);
    m_owner.GetSubpassOwner().SetDirtyCacheCommands();
  }
}

void DescriptorBuffer::OnDescriptorChanged(const SamplerUniform & descriptor) noexcept
{
  if (m_set)
  {
    details::UpdateDescriptorResource(m_context, m_set, descriptor);
    m_owner.GetSubpassOwner().SetDirtyCacheCommands();
  }
}

void DescriptorBuffer::BindToCommandBuffer(const vk::CommandBuffer & buffer,
                                           vk::PipelineLayout pipelineLayout,
                                           VkPipelineBindPoint bindPoint)
{
  if (m_set)
  {
    const VkDescriptorSet set = m_set;
    vkCmdBindDescriptorSets(buffer, bindPoint, pipelineLayout, 0, 1, &set, 0, nullptr);
  }
}

vk::DescriptorSetLayout DescriptorBuffer::GetLayoutHandle() const noexcept
{
  return m_layout;
}

vk::DescriptorSet DescriptorBuffer::GetHandle() const noexcept
{
  return m_set;
}

} // namespace RHI::vulkan
