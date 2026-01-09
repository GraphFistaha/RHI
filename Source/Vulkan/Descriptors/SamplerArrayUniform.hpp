#pragma once

#include <Descriptors/BaseUniform.hpp>
#include <ImageUtils/TextureInterface.hpp>
#include <RHI.hpp>
#include <Utils/SamplerBuilder.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{

struct SamplerArrayUniform final : public ISamplerArrayUniformDescriptor,
                                   public details::BaseUniform
{
  explicit SamplerArrayUniform(Context & ctx, DescriptorBufferLayout & owner, size_t size,
                               VkDescriptorType type, LayoutIndex index, uint32_t arrayIndex = 0);
  virtual ~SamplerArrayUniform() override;
  SamplerArrayUniform(SamplerArrayUniform && rhs) noexcept;
  SamplerArrayUniform & operator=(SamplerArrayUniform && rhs) noexcept;

public: // ISamplerDescriptor interface
  virtual void SetWrapping(RHI::TextureWrapping uWrap, RHI::TextureWrapping vWrap,
                           RHI::TextureWrapping wWrap) noexcept override;
  virtual void SetFilter(RHI::TextureFilteration minFilter,
                         RHI::TextureFilteration magFilter) noexcept override;

public:
  std::vector<VkDescriptorImageInfo> CreateDescriptorInfo() const;
  void TransitLayoutForUsedImages(details::CommandBuffer & commandBuffer, VkImageLayout layout);

public: // IUniformDescriptor interface
  virtual uint32_t GetSet() const noexcept override { return BaseUniform::GetSet(); }
  virtual uint32_t GetBinding() const noexcept override { return BaseUniform::GetBinding(); }
  virtual uint32_t GetArrayIndex() const noexcept override { return BaseUniform::GetArrayIndex(); }

public: // ISamplerArrayUniformDescriptor interface
  virtual void AssignImage(uint32_t index, ITexture * image) override;

public: // IInvalidable interface
  void Invalidate();
  void SetInvalid();

public: // public internal API
  VkSampler GetHandle() const noexcept;
  using BaseUniform::GetDescriptorType;

private:
  std::vector<IInternalTexture *> m_boundTextures;
  VkSampler m_sampler = VK_NULL_HANDLE;
  utils::SamplerBuilder m_builder;
  bool m_invalidSampler = true;
};

} // namespace RHI::vulkan
