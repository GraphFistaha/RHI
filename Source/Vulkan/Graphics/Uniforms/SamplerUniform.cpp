#include "SamplerUniform.hpp"

#include "../../Utils/CastHelper.hpp"
#include "../../VulkanContext.hpp"
#include "../DescriptorsBuffer.hpp"

namespace RHI::vulkan
{
namespace details
{
VkSampler CreateSampler(const VkDevice & device)
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
  return VkSampler(resultSampler);
}
} // namespace details


SamplerUniform::SamplerUniform(Context & ctx, DescriptorBuffer & owner, VkDescriptorType type,
                               uint32_t binding, uint32_t arrayIndex)
  : BaseUniform(ctx, owner, type, binding, arrayIndex)
  , ISamplerUniformDescriptor()
{
}

SamplerUniform::~SamplerUniform()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_sampler, nullptr);
}

SamplerUniform::SamplerUniform(SamplerUniform && rhs) noexcept
  : BaseUniform(std::move(rhs))
  , ISamplerUniformDescriptor()
{
  std::swap(rhs.m_boundTexture, m_boundTexture);
  std::swap(rhs.m_sampler, m_sampler);
  std::swap(rhs.m_invalidSampler, m_invalidSampler);
}

SamplerUniform & SamplerUniform::operator=(SamplerUniform && rhs) noexcept
{
  if (this != &rhs)
  {
    BaseUniform::operator=(std::move(rhs));
    std::swap(rhs.m_boundTexture, m_boundTexture);
    std::swap(rhs.m_sampler, m_sampler);
    std::swap(rhs.m_invalidSampler, m_invalidSampler);
  }
  return *this;
}

VkSampler SamplerUniform::GetHandle() const noexcept
{
  return m_sampler;
}

VkDescriptorImageInfo SamplerUniform::CreateDescriptorInfo() const noexcept
{
  assert(m_sampler);
  assert(m_boundTexture);
  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = m_boundTexture->GetImageView(ImageUsage::Sampler);
  imageInfo.sampler = m_sampler;
  return imageInfo;
}

void SamplerUniform::Invalidate()
{
  if (m_invalidSampler || !m_sampler)
  {
    auto new_sampler = details::CreateSampler(GetContext().GetDevice());
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_sampler, nullptr);
    m_sampler = new_sampler;
    m_invalidSampler = false;
  }
}

void SamplerUniform::SetInvalid()
{
  m_invalidSampler = true;
}

void SamplerUniform::AssignImage(IImageGPU * image)
{
  Invalidate();
  auto && internalImage = dynamic_cast<ITexture *>(image);
  m_boundTexture = internalImage;
  GetDescriptorsBuffer().OnDescriptorChanged(*this);
}

bool SamplerUniform::IsImageAssigned() const noexcept
{
  return m_boundTexture != nullptr;
}

uint32_t SamplerUniform::GetBinding() const noexcept
{
  return BaseUniform::GetBinding();
}

uint32_t SamplerUniform::GetArrayIndex() const noexcept
{
  return BaseUniform::GetArrayIndex();
}
} // namespace RHI::vulkan
