#include "SamplerUniform.hpp"

#include "../VulkanContext.hpp"
#include "DescriptorsBuffer.hpp"

namespace RHI::vulkan
{
namespace details
{
vk::Sampler CreateSampler(const vk::Device & device)
{
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 0;

  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  VkSampler resultSampler;
  if (vkCreateSampler(device, &samplerInfo, nullptr, &resultSampler) != VK_SUCCESS)
    throw std::invalid_argument("failed to create texture sampler!");
  return vk::Sampler(resultSampler);
}
} // namespace details


ImageSampler::ImageSampler(const Context & ctx, DescriptorBuffer & owner, VkDescriptorType type,
                           uint32_t descriptorIndex, uint32_t binding)
  : m_context(ctx)
  , m_owner(&owner)
  , m_type(type)
  , m_descriptorIndex(descriptorIndex)
  , m_binding(binding)
{
  assert(m_owner);
  assert(descriptorIndex != InvalidDescriptorIndex);
}

ImageSampler::~ImageSampler()
{
  if (m_sampler)
    vkDestroySampler(m_context.GetDevice(), m_sampler, nullptr);
}

ImageSampler::ImageSampler(ImageSampler && rhs) noexcept
  : m_context(rhs.m_context)
{
  std::swap(m_owner, rhs.m_owner);
  std::swap(m_type, rhs.m_type);
  std::swap(m_descriptorIndex, rhs.m_descriptorIndex);
  std::swap(m_binding, rhs.m_binding);
  std::swap(rhs.m_view, m_view);
  std::swap(rhs.m_sampler, m_sampler);
  std::swap(rhs.m_invalidSampler, m_invalidSampler);
}

ImageSampler & ImageSampler::operator=(ImageSampler && rhs) noexcept
{
  if (this != &rhs && &rhs.m_context == &m_context)
  {
    std::swap(m_owner, rhs.m_owner);
    std::swap(m_type, rhs.m_type);
    std::swap(m_descriptorIndex, rhs.m_descriptorIndex);
    std::swap(m_binding, rhs.m_binding);
    std::swap(rhs.m_view, m_view);
    std::swap(rhs.m_sampler, m_sampler);
    std::swap(rhs.m_invalidSampler, m_invalidSampler);
  }
  return *this;
}

VkSampler ImageSampler::GetHandle() const noexcept
{
  return m_sampler;
}

VkDescriptorImageInfo ImageSampler::CreateDescriptorInfo() const noexcept
{
  assert(m_sampler);
  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = m_view.GetHandle();
  imageInfo.sampler = m_sampler;
  return imageInfo;
}

void ImageSampler::Invalidate()
{
  if (m_invalidSampler || !m_sampler)
  {
    auto new_sampler = details::CreateSampler(m_context.GetDevice());
    if (m_sampler)
      vkDestroySampler(m_context.GetDevice(), m_sampler, nullptr);
    m_sampler = new_sampler;
    m_invalidSampler = false;
  }
}

void ImageSampler::AssignImage(const IImageGPU & image)
{
  m_view.AssignImage(image);
  assert(m_owner);
  m_owner->OnDescriptorChanged(*this);
}

bool ImageSampler::IsImageAssigned() const noexcept
{
  return m_view.IsImageAssigned();
}

uint32_t ImageSampler::GetDescriptorIndex() const noexcept
{
  return m_descriptorIndex;
}

uint32_t ImageSampler::GetBinding() const noexcept
{
  return m_binding;
}
} // namespace RHI::vulkan
