#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../../Images/Image.hpp"
#include "BaseUniform.hpp"


namespace RHI::vulkan
{

struct SamplerUniform final : public ISamplerUniformDescriptor,
                              private details::BaseUniform
{
  explicit SamplerUniform(Context & ctx, DescriptorBuffer & owner, VkDescriptorType type,
                          uint32_t binding, uint32_t arrayIndex = 0);
  virtual ~SamplerUniform() override;
  SamplerUniform(SamplerUniform && rhs) noexcept;
  SamplerUniform & operator=(SamplerUniform && rhs) noexcept;

public: // ISamplerUniformDescriptor interface
  virtual void AssignImage(IImageGPU * image) override;
  virtual bool IsImageAssigned() const noexcept override;

public:
  virtual uint32_t GetBinding() const noexcept override;
  virtual uint32_t GetArrayIndex() const noexcept override;

public: // IInvalidable interface
  virtual void Invalidate() override;
  virtual void SetInvalid() override;

public: // public internal API
  VkSampler GetHandle() const noexcept;
  Image * GetAttachedImage() const noexcept { return m_attachedImage; }
  VkDescriptorImageInfo CreateDescriptorInfo() const noexcept;
  using BaseUniform::GetDescriptorType;

private:
  Image * m_attachedImage = nullptr;
  VkSampler m_sampler = VK_NULL_HANDLE;
  bool m_invalidSampler = true;
};

} // namespace RHI::vulkan
