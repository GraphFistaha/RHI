#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../ContextualObject.hpp"

namespace RHI::vulkan::details
{
struct CommandBuffer;
} // namespace RHI::vulkan::details

namespace RHI::vulkan
{
/// @brief Base class for all images
struct ImageBase : public IImageGPU,
                   public ContextualObject
{
  virtual ~ImageBase() = default;
  ImageBase(ImageBase && rhs) noexcept;
  ImageBase & operator=(ImageBase && rhs) noexcept;

protected:
  explicit ImageBase(Context & ctx, const ImageDescription & description);

public:
  virtual void UploadImage(const uint8_t * data, const CopyImageArguments & args) override;
  virtual std::future<std::vector<uint8_t>> DownloadImage(const CopyImageArguments & args) override;
  virtual size_t Size() const override;
  virtual ImageDescription GetDescription() const noexcept override;

public:
  void SetImageLayout(details::CommandBuffer & commandBuffer, VkImageLayout newLayout) noexcept;

  VkImage GetHandle() const noexcept { return m_image; }
  VkImageLayout GetLayout() const noexcept { return m_layout; }

  VkImageType GetVulkanImageType() const noexcept;
  VkExtent3D GetVulkanExtent() const noexcept;
  VkFormat GetVulkanFormat() const noexcept;
  VkSampleCountFlagBits GetVulkanSamplesCount() const noexcept;

protected:
  VkImage m_image = VK_NULL_HANDLE;
  VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  ImageDescription m_description;

private:
  ImageBase(const ImageBase &) = delete;
  ImageBase & operator=(const ImageBase &) = delete;
};
} // namespace RHI::vulkan
