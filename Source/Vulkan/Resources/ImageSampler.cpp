#include "ImageSampler.hpp"

#include "../VulkanContext.hpp"

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


ImageGPU_Sampler::ImageGPU_Sampler(const Context & ctx)
  : m_context(ctx)
{
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(ctx.GetGPU(), &properties);
}

ImageGPU_Sampler::~ImageGPU_Sampler()
{
  if (m_sampler)
    vkDestroySampler(m_context.GetDevice(), m_sampler, nullptr);
}

InternalObjectHandle ImageGPU_Sampler::GetHandle() const noexcept
{
  return m_sampler;
}

void ImageGPU_Sampler::Invalidate()
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
} // namespace RHI::vulkan
