#include "SamplerBuilder.hpp"

#include <Utils/CastHelper.hpp>


namespace RHI::vulkan::utils
{

template<>
constexpr inline VkFilter CastInterfaceEnum2Vulkan<VkFilter, RHI::TextureFilteration>(
  RHI::TextureFilteration value)
{
  switch (value)
  {
    case RHI::TextureFilteration::Nearest:
      return VK_FILTER_NEAREST;
    case RHI::TextureFilteration::Linear:
      return VK_FILTER_LINEAR;
    default:
      return VK_FILTER_MAX_ENUM;
  }
}

template<>
constexpr inline VkSamplerAddressMode CastInterfaceEnum2Vulkan<
  VkSamplerAddressMode, RHI::TextureWrapping>(RHI::TextureWrapping value)
{
  switch (value)
  {
    case RHI::TextureWrapping::Repeat:
      return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case RHI::TextureWrapping::MirroredRepeat:
      return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case RHI::TextureWrapping::ClampToEdge:
      return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case RHI::TextureWrapping::ClampToBorder:
      return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    default:
      return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
  }
}

} // namespace RHI::vulkan::utils

namespace RHI::vulkan::utils
{

VkSampler SamplerBuilder::Make(const VkDevice & device) const
{
  VkSampler resultSampler;
  if (vkCreateSampler(device, &m_createInfo, nullptr, &resultSampler) != VK_SUCCESS)
    throw std::invalid_argument("failed to create texture sampler!");
  return resultSampler;
}

void SamplerBuilder::Reset()
{
  m_createInfo = VkSamplerCreateInfo{};
  m_createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  m_createInfo.magFilter = VK_FILTER_NEAREST;
  m_createInfo.minFilter = VK_FILTER_NEAREST;
  m_createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  m_createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  m_createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  m_createInfo.anisotropyEnable = VK_FALSE;
  m_createInfo.maxAnisotropy = 0;
  m_createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  m_createInfo.unnormalizedCoordinates = VK_FALSE;
  m_createInfo.compareEnable = VK_FALSE;
  m_createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  m_createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  m_createInfo.mipLodBias = 0.0f;
  m_createInfo.minLod = 0.0f;
  m_createInfo.maxLod = 0.0f;
}

void SamplerBuilder::SetFilter(RHI::TextureFilteration minFilter,
                               RHI::TextureFilteration magFilter) noexcept
{
  m_createInfo.minFilter = CastInterfaceEnum2Vulkan<VkFilter>(minFilter);
  m_createInfo.magFilter = CastInterfaceEnum2Vulkan<VkFilter>(magFilter);
}

void SamplerBuilder::SetTextureWrapping(RHI::TextureWrapping uWrapping,
                                        RHI::TextureWrapping vWrapping,
                                        RHI::TextureWrapping wWrapping)
{
  m_createInfo.addressModeU = CastInterfaceEnum2Vulkan<VkSamplerAddressMode>(uWrapping);
  m_createInfo.addressModeV = CastInterfaceEnum2Vulkan<VkSamplerAddressMode>(vWrapping);
  m_createInfo.addressModeW = CastInterfaceEnum2Vulkan<VkSamplerAddressMode>(wWrapping);
}

} // namespace RHI::vulkan::utils
