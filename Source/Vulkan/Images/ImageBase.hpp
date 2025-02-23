#pragma once
#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
namespace details
{
struct CommandBuffer;
}
} // namespace RHI::vulkan

namespace RHI::vulkan
{
/// @brief Base class for all images
struct ImageBase : public IImageGPU,
                   public OwnedBy<Context>
{
  virtual ~ImageBase() = default;
  ImageBase(ImageBase && rhs) noexcept;
  ImageBase & operator=(ImageBase && rhs) noexcept;

protected:
  explicit ImageBase(Context & ctx, const ImageCreateArguments & description);

public:
  virtual std::future<UploadResult> UploadImage(const uint8_t * data,
                                                const CopyImageArguments & args) override;
  virtual std::future<DownloadResult> DownloadImage(HostImageFormat format,
                                                    const ImageRegion & args) override;
  virtual size_t Size() const override;
  virtual ImageCreateArguments GetDescription() const noexcept override;

public:
  void TransferLayout(details::CommandBuffer & commandBuffer, VkImageLayout newLayout) noexcept;
  void SetImageLayoutBeforeRenderPass(VkImageLayout newLayout) noexcept;
  void SetImageLayoutAfterRenderPass(VkImageLayout newLayout) noexcept;

  VkImage GetHandle() const noexcept { return m_image; }
  VkImageLayout GetLayout() const noexcept { return m_layout; }

  VkImageType GetVulkanImageType() const noexcept;
  VkExtent3D GetVulkanExtent() const noexcept;
  VkFormat GetVulkanFormat() const noexcept;
  VkSampleCountFlagBits GetVulkanSamplesCount() const noexcept;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

protected:
  std::mutex m_layoutLock;
  VkImage m_image = VK_NULL_HANDLE;
  VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  ImageCreateArguments m_description;

private:
  ImageBase(const ImageBase &) = delete;
  ImageBase & operator=(const ImageBase &) = delete;
};

} // namespace RHI::vulkan
