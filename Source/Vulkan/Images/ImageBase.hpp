#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
struct Transferer;
namespace details
{
struct CommandBuffer;
}
} // namespace RHI::vulkan

namespace RHI::vulkan
{
/// @brief Base class for all images
struct ImageBase : public IImageGPU
{
  virtual ~ImageBase() = default;
  ImageBase(ImageBase && rhs) noexcept;
  ImageBase & operator=(ImageBase && rhs) noexcept;

protected:
  explicit ImageBase(const Context & ctx, Transferer * transferer,
                     const ImageDescription & description);

public:
  virtual void UploadImage(const uint8_t * data, const CopyImageArguments & args) override;
  //void DownloadImage(const CopyImageArguments & args, uint8_t * data);
  virtual size_t Size() const override;
  virtual ImageDescription GetDescription() const noexcept override;

public:
  friend const Context & GetContextFromImage(const ImageBase & image) noexcept;
  void SetImageLayout(details::CommandBuffer & commandBuffer, VkImageLayout newLayout) noexcept;

  VkImage GetHandle() const noexcept { return m_image; }
  VkImageLayout GetLayout() const noexcept { return m_layout; }

  VkImageType GetVulkanImageType() const noexcept;
  VkExtent3D GetVulkanExtent() const noexcept;
  VkFormat GetVulkanFormat() const noexcept;
  VkSampleCountFlagBits GetVulkanSamplesCount() const noexcept;

protected:
  const Context & m_context;
  Transferer * m_transferer = nullptr;

  VkImage m_image = VK_NULL_HANDLE;
  VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  ImageDescription m_description;

private:
  ImageBase(const ImageBase &) = delete;
  ImageBase & operator=(const ImageBase &) = delete;
};
} // namespace RHI::vulkan
