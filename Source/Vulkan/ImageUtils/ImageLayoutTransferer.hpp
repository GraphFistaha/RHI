#pragma once
#include <unordered_map>

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Memory/MemoryBlock.hpp"

namespace RHI::vulkan
{
struct Context;
struct BufferedTexture;
namespace details
{
struct CommandBuffer;
}
} // namespace RHI::vulkan

namespace RHI::vulkan
{
struct ImageLayoutTransferer final
{
  explicit ImageLayoutTransferer(VkImage image);
  ~ImageLayoutTransferer() = default;
  ImageLayoutTransferer(ImageLayoutTransferer && rhs) noexcept;
  ImageLayoutTransferer & operator=(ImageLayoutTransferer && rhs) noexcept;
  RESTRICTED_COPY(ImageLayoutTransferer);

public:
  void TransferLayout(VkImageLayout newLayout) noexcept;
  void TransferLayout(details::CommandBuffer & commandBuffer, VkImageLayout newLayout) noexcept;

  VkImage GetHandle() const noexcept { return m_image; }
  VkImageLayout GetLayout() const noexcept { return m_layout; }

protected:
  VkImage m_image = VK_NULL_HANDLE;                                ///< handle of vulkan image
  std::atomic<VkImageLayout> m_layout = VK_IMAGE_LAYOUT_UNDEFINED; ///< image layout
};

} // namespace RHI::vulkan
