#pragma once
#include <memory>

#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct FramebufferAttachment final
{
  FramebufferAttachment() = default;
  explicit FramebufferAttachment(VkImageView imageView, VkFormat format, VkImageLayout layout,
                                 VkSampleCountFlagBits samplesCount) noexcept
    : m_imageView(imageView)
    , m_imageFormat(format)
    , m_imageLayout(layout)
    , m_samplesCount(samplesCount)
  {
  }

  VkImageView GetImageView() const noexcept { return m_imageView; }
  VkFormat GetImageFormat() const noexcept { return m_imageFormat; }
  VkImageLayout GetImageLayout() const noexcept { return m_imageLayout; }
  VkSampleCountFlagBits GetSamplesCount() const noexcept { return m_samplesCount; }

  bool operator==(const FramebufferAttachment & rhs) const noexcept
  {
    return m_imageFormat == rhs.m_imageFormat &&
           m_imageLayout == rhs.m_imageLayout && m_samplesCount == rhs.m_samplesCount;
  }

private:
  VkImageView m_imageView = VK_NULL_HANDLE;
  VkFormat m_imageFormat = VK_FORMAT_UNDEFINED;
  VkImageLayout m_imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkSampleCountFlagBits m_samplesCount = VK_SAMPLE_COUNT_1_BIT;
};
} // namespace RHI::vulkan


// hasher
template<>
struct std::hash<RHI::vulkan::FramebufferAttachment>
{
  std::size_t operator()(const RHI::vulkan::FramebufferAttachment & attachment) const noexcept
  {
    std::size_t h1 = std::hash<size_t>{}(attachment.GetImageFormat());
    std::size_t h2 = std::hash<size_t>{}(attachment.GetImageLayout());
    std::size_t h3 = std::hash<size_t>{}(attachment.GetSamplesCount());
    auto hash_combine = [](size_t h1, size_t h2)
    {
      return h1 ^ (h2 << 1);
    };
    return hash_combine(hash_combine(h1, h2), h3);
  }
};
