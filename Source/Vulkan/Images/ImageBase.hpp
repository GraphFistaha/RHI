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

namespace RHI::vulkan::details
{
struct ImageBase
{
protected:
  explicit ImageBase(const Context & ctx, Transferer & transferer);
  virtual ~ImageBase() = default;
  ImageBase(ImageBase && rhs) noexcept;
  ImageBase & operator=(ImageBase && rhs) noexcept;

public:
  void UploadImage(const uint8_t * data, const CopyImageArguments & args);
  VkImage GetHandle() const noexcept { return m_image; }
  void SetImageLayout(details::CommandBuffer & commandBuffer, VkImageLayout newLayout) noexcept;
  ImageExtent GetExtent() const noexcept;


protected:
  const Context & m_context;
  Transferer * m_transferer = nullptr;

  VkImage m_image = VK_NULL_HANDLE;
  VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkFormat m_internalFormat = VK_FORMAT_UNDEFINED;
  uint32_t m_mipLevels;

private:
  ImageBase(const ImageBase &) = delete;
  ImageBase & operator=(const ImageBase &) = delete;
};
} // namespace RHI::vulkan::details
