#include "SamplerArrayUniform.hpp"

#include <Descriptors/DescriptorBufferLayout.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{

SamplerArrayUniform::SamplerArrayUniform(Context & ctx, DescriptorBufferLayout & owner, size_t size,
                                         VkDescriptorType type, LayoutIndex index,
                                         uint32_t arrayIndex)
  : BaseUniform(ctx, owner, type, index, arrayIndex)
{
  m_builder.Reset();
  m_boundTextures.resize(size, dynamic_cast<IInternalTexture *>(ctx.GetNullTexture()));
}

SamplerArrayUniform::~SamplerArrayUniform()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_sampler, nullptr);
}

SamplerArrayUniform::SamplerArrayUniform(SamplerArrayUniform && rhs) noexcept
  : BaseUniform(std::move(rhs))
{
  std::swap(rhs.m_boundTextures, m_boundTextures);
  std::swap(rhs.m_sampler, m_sampler);
  std::swap(rhs.m_invalidSampler, m_invalidSampler);
  std::swap(rhs.m_builder, m_builder);
}

SamplerArrayUniform & SamplerArrayUniform::operator=(SamplerArrayUniform && rhs) noexcept
{
  if (this != &rhs)
  {
    BaseUniform::operator=(std::move(rhs));
    std::swap(rhs.m_boundTextures, m_boundTextures);
    std::swap(rhs.m_sampler, m_sampler);
    std::swap(rhs.m_invalidSampler, m_invalidSampler);
    std::swap(rhs.m_builder, m_builder);
  }
  return *this;
}

VkSampler SamplerArrayUniform::GetHandle() const noexcept
{
  return m_sampler;
}

std::vector<VkDescriptorImageInfo> SamplerArrayUniform::CreateDescriptorInfo() const
{
  assert(m_sampler);
  std::vector<VkDescriptorImageInfo> infos;
  for (auto * texture : m_boundTextures)
  {
    assert(texture != nullptr);
    auto && imageInfo = infos.emplace_back();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture->GetImageView();
    imageInfo.sampler = m_sampler;
  }
  return infos;
}

void SamplerArrayUniform::TransitLayoutForUsedImages(details::CommandBuffer & commandBuffer,
                                                     VkImageLayout layout)
{
  for (auto * texture : m_boundTextures)
  {
    if (texture)
      texture->TransferLayout(commandBuffer, layout);
  }
}

void SamplerArrayUniform::Invalidate()
{
  if (m_invalidSampler || !m_sampler)
  {
    auto new_sampler = m_builder.Make(GetContext().GetGpuConnection().GetDevice());
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "new VkSampler has been created");
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_sampler, nullptr);
    m_sampler = new_sampler;
    m_invalidSampler = false;
  }
}

void SamplerArrayUniform::SetInvalid()
{
  m_invalidSampler = true;
}

void SamplerArrayUniform::AssignImage(uint32_t index, ITexture * image)
{
  m_boundTextures[index] = image ? dynamic_cast<IInternalTexture *>(image)
                                 : dynamic_cast<IInternalTexture *>(GetContext().GetNullTexture());
  GetLayout().GetConfiguration().GetSubpass().OnDescriptorChanged(*this);
}


void SamplerArrayUniform::SetWrapping(RHI::TextureWrapping uWrap, RHI::TextureWrapping vWrap,
                                      RHI::TextureWrapping wWrap) noexcept
{
  m_builder.SetTextureWrapping(uWrap, vWrap, wWrap);
  m_invalidSampler = true;
}

void SamplerArrayUniform::SetFilter(RHI::TextureFilteration minFilter,
                                    RHI::TextureFilteration magFilter) noexcept
{
  m_builder.SetFilter(minFilter, magFilter);
  m_invalidSampler = true;
}
} // namespace RHI::vulkan
