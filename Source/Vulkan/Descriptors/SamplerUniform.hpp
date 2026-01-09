#pragma once

#include <Descriptors/BaseUniform.hpp>
#include <ImageUtils/TextureInterface.hpp>
#include <RHI.hpp>
#include <Utils/SamplerBuilder.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{

struct SamplerUniform final : public ISamplerUniformDescriptor,
                              public details::BaseUniform
{
  explicit SamplerUniform(Context & ctx, DescriptorBufferLayout & owner, VkDescriptorType type,
                          LayoutIndex index, uint32_t arrayIndex = 0);
  virtual ~SamplerUniform() override;
  SamplerUniform(SamplerUniform && rhs) noexcept;
  SamplerUniform & operator=(SamplerUniform && rhs) noexcept;

public: // ISamplerDescriptor interface
  virtual void SetWrapping(RHI::TextureWrapping uWrap, RHI::TextureWrapping vWrap,
                           RHI::TextureWrapping wWrap) noexcept override;
  virtual void SetFilter(RHI::TextureFilteration minFilter,
                         RHI::TextureFilteration magFilter) noexcept override;

public: // ISamplerUniformDescriptor interface
  virtual void AssignImage(ITexture * image) override;

public:
  std::vector<VkDescriptorImageInfo> CreateDescriptorInfo() const;
  void TransitLayoutForUsedImages(details::CommandBuffer & commandBuffer, VkImageLayout layout);

public: // IUniformDescriptor interface
  virtual uint32_t GetSet() const noexcept override { return BaseUniform::GetSet(); }
  virtual uint32_t GetBinding() const noexcept override { return BaseUniform::GetBinding(); }
  virtual uint32_t GetArrayIndex() const noexcept override { return BaseUniform::GetArrayIndex(); }

public: // IInvalidable interface
  void Invalidate();
  void SetInvalid();

public: // public internal API
  VkSampler GetHandle() const noexcept;
  using BaseUniform::GetDescriptorType;

private:
  IInternalTexture * m_boundTexture = nullptr;
  VkSampler m_sampler = VK_NULL_HANDLE;
  utils::SamplerBuilder m_builder;
  bool m_invalidSampler = true;
};

} // namespace RHI::vulkan
