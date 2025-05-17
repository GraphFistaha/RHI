#include "SamplerUniform.hpp"

#include "../../Utils/CastHelper.hpp"
#include "../../VulkanContext.hpp"
#include "../DescriptorsBuffer.hpp"

namespace RHI::vulkan
{

SamplerUniform::SamplerUniform(Context & ctx, DescriptorBuffer & owner, VkDescriptorType type,
                               uint32_t binding, uint32_t arrayIndex)
  : BaseUniform(ctx, owner, type, binding, arrayIndex)
  , ISamplerUniformDescriptor()
{
  m_builder.Reset();
  AssignImage(nullptr);
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
  std::swap(rhs.m_builder, m_builder);
}

SamplerUniform & SamplerUniform::operator=(SamplerUniform && rhs) noexcept
{
  if (this != &rhs)
  {
    BaseUniform::operator=(std::move(rhs));
    std::swap(rhs.m_boundTexture, m_boundTexture);
    std::swap(rhs.m_sampler, m_sampler);
    std::swap(rhs.m_invalidSampler, m_invalidSampler);
    std::swap(rhs.m_builder, m_builder);
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
  imageInfo.imageView = m_boundTexture->GetImageView();
  imageInfo.sampler = m_sampler;
  return imageInfo;
}

void SamplerUniform::Invalidate()
{
  if (m_invalidSampler || !m_sampler)
  {
    auto new_sampler = m_builder.Make(GetContext().GetDevice());
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "new VkSampler has been created");
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_sampler, nullptr);
    m_sampler = new_sampler;
    m_invalidSampler = false;
  }
}

void SamplerUniform::SetInvalid()
{
  m_invalidSampler = true;
}

void SamplerUniform::AssignImage(ITexture * image)
{
  m_boundTexture = image ? dynamic_cast<IInternalTexture *>(image)
                         : dynamic_cast<IInternalTexture *>(GetContext().GetNullTexture());
}

bool SamplerUniform::IsImageAssigned() const noexcept
{
  return m_boundTexture != dynamic_cast<IInternalTexture *>(GetContext().GetNullTexture()) ||
         !m_boundTexture;
}

void SamplerUniform::SetWrapping(RHI::TextureWrapping uWrap, RHI::TextureWrapping vWrap,
                                 RHI::TextureWrapping wWrap) noexcept
{
  m_builder.SetTextureWrapping(uWrap, vWrap, wWrap);
}

void SamplerUniform::SetFilter(RHI::TextureFilteration minFilter,
                               RHI::TextureFilteration magFilter) noexcept
{
  m_builder.SetFilter(minFilter, magFilter);
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
