#pragma once
#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Memory/MemoryBlock.hpp"

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
struct Image final : public OwnedBy<Context>
{
  explicit Image(Context & ctx, const ImageCreateArguments & description);
  ~Image();
  Image(Image && rhs) noexcept;
  Image & operator=(Image && rhs) noexcept;

public:
  std::future<UploadResult> UploadImage(const uint8_t * data, const CopyImageArguments & args);
  std::future<DownloadResult> DownloadImage(HostImageFormat format, const ImageRegion & args);
  size_t Size() const;
  ImageCreateArguments GetDescription() const noexcept;

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
  std::mutex m_layoutLock;                            ///< mutex used to sync m_layout setting
  VkImage m_image = VK_NULL_HANDLE;                   ///< handle of vulkan image
  VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED; ///< image layout
  ImageCreateArguments m_description;                 ///< description of image
  /// memory block for image. If none then Image doesn't own m_image
  memory::MemoryBlock m_memBlock;

private:
  Image(const Image &) = delete;
  Image & operator=(const Image &) = delete;
};

} // namespace RHI::vulkan
