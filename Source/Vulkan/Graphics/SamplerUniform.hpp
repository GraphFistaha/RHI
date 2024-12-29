#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Images/ImageView.hpp"

namespace RHI::vulkan
{
struct Context;
struct DescriptorBuffer;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct ImageSampler final : public ISamplerUniformDescriptor
{
  static constexpr uint32_t InvalidDescriptorIndex = -1;

  explicit ImageSampler(const Context & ctx, DescriptorBuffer & owner, VkDescriptorType type,
                        uint32_t descriptorIndex, uint32_t binding);
  virtual ~ImageSampler() override;
  ImageSampler(ImageSampler && rhs) noexcept;
  ImageSampler & operator=(ImageSampler && rhs) noexcept;

public: // ISamplerUniformDescriptor interface
  virtual void AssignImage(const IImageGPU & image) override;
  virtual bool IsImageAssigned() const noexcept override;

public:
  virtual uint32_t GetDescriptorIndex() const noexcept override;
  virtual uint32_t GetBinding() const noexcept override;
  VkDescriptorType GetDescriptorType() const noexcept { return m_type; }

public: // IInvalidable interface
  virtual void Invalidate() override;

public: // public internal API
  VkSampler GetHandle() const noexcept;
  VkImageView GetImageView() const noexcept { return m_view.GetHandle(); }
  VkDescriptorImageInfo CreateDescriptorInfo() const noexcept;

private:
  const Context & m_context;
  DescriptorBuffer * m_owner;
  VkDescriptorType m_type;
  uint32_t m_descriptorIndex = InvalidDescriptorIndex;
  uint32_t m_binding = InvalidDescriptorIndex;

  ImageGPU_View m_view{m_context};
  vk::Sampler m_sampler = VK_NULL_HANDLE;
  bool m_invalidSampler = true;
};

} // namespace RHI::vulkan
