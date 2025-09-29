#include "DescriptorBufferLayout.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>

#include "../Graphics/SubpassConfiguration.hpp"
#include "../VulkanContext.hpp"

namespace RHI::vulkan::details
{
VkDescriptorPool CreateDescriptorPool(const Context & ctx,
                                      const std::vector<VkDescriptorPoolSize> & poolSizes)
{
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
    throw std::runtime_error("failed to create VkDescriptorPool!");
  return VkDescriptorPool(c_pool);
}

VkDescriptorSet CreateDescriptorSet(const Context & ctx, VkDescriptorPool pool,
                                    VkDescriptorSetLayout layout)
{
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &layout;

  VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
  if (vkAllocateDescriptorSets(ctx.GetDevice(), &allocInfo, &descriptor_set) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate VkDescriptorSet!");
  return VkDescriptorSet(descriptor_set);
}


} // namespace RHI::vulkan::details

namespace RHI::vulkan
{

DescriptorBufferLayout::DescriptorBufferLayout(Context & ctx, SubpassConfiguration & owner)
  : OwnedBy<Context>(ctx)
  , OwnedBy<SubpassConfiguration>(owner)
{
}

DescriptorBufferLayout::~DescriptorBufferLayout()
{
  for (auto layout : m_layouts)
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(layout, nullptr);
}

void DescriptorBufferLayout::TransitLayoutForUsedImages(details::CommandBuffer & commandBuffer)
{
  for (auto && sampler : m_samplerDescriptors)
  {
    VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    auto && imagePtr = sampler.GetAttachedImage();
    if (imagePtr)
      imagePtr->TransferLayout(commandBuffer, layout);
  }
}

void DescriptorBufferLayout::SetInvalid()
{
  std::fill(m_invalidLayouts.begin(), m_invalidLayouts.end(), ValidityFlag::NotValid);
}

void DescriptorBufferLayout::Invalidate()
{
  size_t setsCount = m_layouts.size();
  assert(setsCount == m_invalidLayouts.size() && setsCount == m_builders.size());

  for (size_t i = 0; i < setsCount; ++i)
  {
    if (m_invalidLayouts[i] == ValidityFlag::NotValid || !m_layouts[i])
    {
      auto newLayout = m_builders[i].Make(GetContext().GetDevice());
      GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_layouts[i], nullptr);
      m_layouts[i] = newLayout;
      m_invalidLayouts[i] = ValidityFlag::Valid;
      GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "VkDescriptorSetLayout has been rebuilt");
    }
  }

  for (auto && uniform : m_bufferUniformDescriptors)
    uniform.Invalidate();

  for (auto && uniform : m_samplerDescriptors)
    uniform.Invalidate();
}

void DescriptorBufferLayout::DeclareBufferUniformsArray(LayoutIndex index, ShaderType shaderStage,
                                                        uint32_t size,
                                                        IBufferUniformDescriptor * outArray[])
{
  const VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  DeclareDescriptorsArray(index, type, shaderStage, size);

  for (uint32_t i = 0; i < size; ++i)
  {
    auto && [it, inserted] = m_indexedDescriptors.insert({index, {}});
    auto && newDescriptor =
      m_bufferUniformDescriptors.emplace_back(GetContext(), *this, type, index, i);
    it->second.push_back(&newDescriptor);
    outArray[i] = &newDescriptor;
  }
}

void DescriptorBufferLayout::DeclareSamplerUniformsArray(LayoutIndex index, ShaderType shaderStage,
                                                         uint32_t size,
                                                         ISamplerUniformDescriptor * outArray[])
{
  const VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  DeclareDescriptorsArray(index, type, shaderStage, size);

  for (uint32_t i = 0; i < size; ++i)
  {
    auto && [it, inserted] = m_indexedDescriptors.insert({index, {}});
    auto && newDescriptor = m_samplerDescriptors.emplace_back(GetContext(), *this, type, index, i);
    it->second.push_back(&newDescriptor);
    outArray[i] = &newDescriptor;
  }
}

std::pair<VkDescriptorPool, std::vector<VkDescriptorSet>> DescriptorBufferLayout::
  AllocDescriptorSets() const
{
  std::vector<VkDescriptorPoolSize> poolSizes;
  poolSizes.reserve(m_setsInfo.size());
  for (auto && [type, metaInfos] : m_setsInfo)
  {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = type;
    poolSize.descriptorCount = std::accumulate(metaInfos.begin(), metaInfos.end(), 0,
                                               [](uint32_t acc, const DescriptorMetaInfo & info)
                                               { return acc + info.arraySize; });
    poolSizes.push_back(poolSize);
  }
  auto pool = details::CreateDescriptorPool(GetContext(), poolSizes);
  std::vector<VkDescriptorSet> sets;
  sets.reserve(m_layouts.size());
  for (auto && layout : m_layouts)
    sets.push_back(details::CreateDescriptorSet(GetContext(), pool, layout));
  return {pool, std::move(sets)};
}

uint32_t DescriptorBufferLayout::GetCountOfOneTypeDescriptors(VkDescriptorType type) const
{
  uint32_t result = 0;
  auto it = m_setsInfo.find(type);
  if (it != m_setsInfo.end())
  {
    result = std::accumulate(it->second.begin(), it->second.end(), 0,
                             [](uint32_t acc, const DescriptorMetaInfo & info)
                             { return acc + info.arraySize; });
  }
  return result;
}

const std::vector<VkDescriptorSetLayout> & DescriptorBufferLayout::GetHandles() const & noexcept
{
  return m_layouts;
}

void DescriptorBufferLayout::DeclareDescriptorsArray(const LayoutIndex & index,
                                                     VkDescriptorType type, ShaderType shaderStage,
                                                     uint32_t size)
{
  const uint32_t setIdx = index.set;
  while (m_layouts.size() <= setIdx)
    m_layouts.push_back(VK_NULL_HANDLE);
  while (m_builders.size() <= setIdx)
    m_builders.emplace_back();
  while (m_invalidLayouts.size() <= setIdx)
    m_invalidLayouts.emplace_back(ValidityFlag::NotValid);

  m_builders[setIdx].DeclareDescriptorsArray(index.binding, type, shaderStage, size);
  m_setsInfo[type].emplace_back(index, size);
  m_invalidLayouts[setIdx] = ValidityFlag::NotValid;
}

} // namespace RHI::vulkan
